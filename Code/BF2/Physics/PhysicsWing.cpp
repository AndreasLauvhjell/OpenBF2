// Wing lift and airflow integration. [INFERRED] Source path.

#include "PhysicsWing.h"

#include <math.h>

#include "BasicPhysicsSystem.h"                              // getGravity / getWind / getAirDensityZeroAtHeight
#include "HeightmapCluster.h"                                // getWaterLevelInWorldCoords (+0x98)
#include "../Core/System.h"                                  // WorldPref::getTickCount
#include "../Game/Objects/IObject.h"
#include "../Game/Objects/Game/Wing.h"                       // calculateNeutralLift, m_liftRegulator
#include "../Game/Objects/RotationalBundleLeaves.h"          // WingTemplate's fields

#include "../Core/Ids.h"
#include "../Game/Objects/IObject.h"
#include "../Game/Objects/ObjectLayoutAccess.h"              // the object's +0x04 flags word
#include "../Game/Utility/Vars.h"

namespace dice {
namespace hfe {
namespace world {

extern IBase* basicPhysicsSystem;                            // ds:0x8B2784C - Core/Globals.cpp
extern IBase* heightmapCluster;                              // ds:0x8ABAF24 - Core/Globals.cpp

namespace {
    // The TU's static initialiser: the sleep overrides consult them first (this TU's own fetch of
    // the same two vars PhysicsSpring's TU reads).
    int* const g_wingForceSleeping = hfe::Vars::getInt("phy-force-sleeping", 0);   // ds:0x8AB0868
    int* const g_wingForceAwake    = hfe::Vars::getInt("phy-force-awake", 0);      // ds:0x8AB086C
    float* const g_phyWingWaterMod = hfe::Vars::getFloat("phy-wing-water-mod", 10.0f);   // ds:0x8AB0870
}

// PhysicsNode(); the vptr; count 1; the matrix zeroed then its diagonal 1.0
// (identity); the object; the template; +0x230 = -1.
PhysicsWing::PhysicsWing(const WingTemplate* wingTemplate, const IObject* object)
    : PhysicsNode()
    , m_refCount(1)                                                                  // +0x22C
    , m_startTick(-1)                                                                 // +0x230
    , m_template(wingTemplate)                                                       // +0x234
    , m_object2(object)                                                              // +0x238
{
    for (int i = 0; i < 16; ++i)
        m_matrix.m[i] = 0.0f;                                                        // +0x23C..+0x278
    m_matrix.m[0] = m_matrix.m[5] = m_matrix.m[10] = m_matrix.m[15] = 1.0f;
}

PhysicsWing::~PhysicsWing() {}                                                       // 0x82AC590: ~PhysicsNode

// The quartet - 0x82AC5B0 / 0x82AC5D0 / 0x82AC5E0 / 0x82AC620.
void PhysicsWing::addRef()  { ++m_refCount; }
int  PhysicsWing::getRef()  { return m_refCount; }
int  PhysicsWing::release()
{
    const int left = m_refCount - 1;
    const bool last = (m_refCount == 1);
    m_refCount = left;
    if (last)
        delete this;                                                                 // vslot +0x14
    return left;
}

void* PhysicsWing::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IPhysicsNode)
        return const_cast<PhysicsWing*>(this);
    return 0;
}


void PhysicsWing::reset(bool full)
{
    PhysicsNode::reset(full);
    m_startTick = -1;
}

// The sleep quartet - phy-force-awake == 1 pins the node awake (sleepiness = the timeout, the object
// ticks), phy-force-sleeping == 1 pins it asleep; otherwise the object's 0x800 tick flag follows the
// sleepiness sign, and setIsAwake reloads the timeout (raising the flag when the node was asleep).
void PhysicsWing::setIsSleeping()
{
    if (*g_wingForceAwake == 1)
    {
        m_sleepiness = m_sleepTimeout;                                               // +0x68 <- +0x64
        m_object->updateFlags(0x800u, 0u);                                           // +0x10 object
        return;
    }
    m_sleepiness = 0;
    m_object->updateFlags(0u, 0x800u);
}

void PhysicsWing::setSleepiness(int sleepiness)
{
    if (*g_wingForceAwake == 1)
    {
        m_sleepiness = m_sleepTimeout;
        m_object->updateFlags(0x800u, 0u);
        return;
    }
    if (*g_wingForceSleeping == 1)
    {
        m_sleepiness = 0;
        m_object->updateFlags(0u, 0x800u);
        return;
    }
    if (sleepiness <= 0)
    {
        if (object_detail::LayoutAccess::flags(m_object) & 0x800u)                   // object +0x04
            m_object->updateFlags(0u, 0x800u);
    }
    else
    {
        if (!(object_detail::LayoutAccess::flags(m_object) & 0x800u))
            m_object->updateFlags(0x800u, 0u);
    }
    m_sleepiness = sleepiness;
}

void PhysicsWing::setIsSleepier()
{
    if (*g_wingForceAwake == 1)
        return;
    if (m_sleepiness > 1)
    {
        --m_sleepiness;
    }
    else if (m_sleepiness == 1)
    {
        m_sleepiness = 0;
        m_object->updateFlags(0u, 0x800u);
    }
}

void PhysicsWing::setIsAwake()
{
    if (*g_wingForceSleeping == 1)
        return;
    if (m_sleepiness <= 0)
        m_object->updateFlags(0x800u, 0u);
    m_sleepiness = m_sleepTimeout;
}


// Returns lift from airflow speed, surface-normal angle and scale; zero speed yields zero.
// The second Vec3 is unused. Preserve the angle curve and repeated upper clamp.
float calculateLift(const Vec3& airflow, const Vec3&, const Vec3& surfaceNormal, float scale)
{
    const float speed = sqrtf(airflow.x * airflow.x + airflow.y * airflow.y + airflow.z * airflow.z);
    if (speed == 0.0f)
        return 0.0f;

    const float inv = 1.0f / speed;
    Vec3 dir;
    dir.x = airflow.x * inv;
    dir.y = airflow.y * inv;
    dir.z = airflow.z * inv;

    const float dot = dir.z * surfaceNormal.z + dir.x * surfaceNormal.x + dir.y * surfaceNormal.y;

    float curve = 0.0f;
    if (dot != 0.0f)
    {
        float t = dot;
        if (t > 1.0f)  t = 1.0f;
        if (t < -1.0f) t = -1.0f;
        if (t > 1.0f)  t = 1.0f;                                 // dead, as compiled
        const float degrees = asinf(t) * 57.295776f;
        if (fabsf(degrees) < 45.0f)
        {
            const float fold = (degrees < 0.0f) ? degrees + 45.0f : 45.0f - degrees;
            curve = fold * degrees / 506.25f;                    // 45*45/4 - peaks at 1 at 22.5 deg
        }
    }

    return (curve * 0.75f + dot * 0.25f) * speed * speed * scale * 0.0024999999f;
}

// Applies lift along the wing's local Y axis after its optional start delay; dt is unused.
// Follows root sleepiness and applies force through the top fixed parent. Airflow subtracts wind
// only above water; altitude and underwater modifiers scale lift before the +/-200 clamp.
// The regulator multiplies neutral lift by wingToRegulatorRatio before adding saved lift;
// preserve the expression rather than combining the two distinct lift terms.
void PhysicsWing::updatePhysics(float)                                               // dt is unread
{
    // 1. the start delay.
    if (m_template->m_startDelay > 0.0f)                                             // template +0x238
    {
        if (m_startTick == -1)                                                       // +0x230
        {
            m_startTick = static_cast<int>(30.0f * m_template->m_startDelay
                                           + static_cast<float>(WorldPref::getTickCount()));
            return;
        }
        // unsigned compare, as compiled (`ja`) - the deadline wraps with the tick counter.
        if (static_cast<unsigned int>(m_startTick)
                > static_cast<unsigned int>(WorldPref::getTickCount()))
            return;
    }

    // 2. the sleep follow.
    if (IPhysicsNode* root = getRootNode())                                          // +0x150
        setSleepiness(root->getSleepiness());                                        // +0x124 <- +0x128
    if (getIsSleeping())                                                             // +0x11C
        return;

    // 3. the lift scale. (The ELF reaches the template through the wing object's +0x1B0; this
    //    class's own +0x234 is the same pointer - WingTemplate::setPhysicsNodeComponent passes
    //    itself and builds the object from itself.)
    const float liftScale = m_template->m_flapLift + m_template->m_wingLift;         // +0x218 + +0x214
    const float scale = static_cast<IPhysicsSystem*>(basicPhysicsSystem)->getGravity()   // +0x20
                      * liftScale / -9.8199997f;

    PhysicsNode* const top = getTopFixParent();
    const float* const tm = top->getAbsoluteTransformation().m;                      // +0x3C
    const float* const rm = getRelativeTransformation().m;                           // +0x44

    Vec3 local;
    local.x = m_template->m_positionOffset.x + rm[12];                               // template +0x220
    local.y = m_template->m_positionOffset.y + rm[13];
    local.z = m_template->m_positionOffset.z + rm[14];

    Vec3 offset;
    offset.x = tm[4] * local.y + tm[0] * local.x + tm[8]  * local.z;
    offset.y = tm[9] * local.z + (tm[5] * local.y + tm[1] * local.x);
    offset.z = tm[10] * local.z + tm[6] * local.y + tm[2] * local.x;

    const Vec3& parentPosition = m_object->getParent()->getAbsolutePosition();        // +0x28, slot 27
    Vec3 at;
    at.x = parentPosition.x + offset.x;
    at.y = parentPosition.y + offset.y;
    at.z = parentPosition.z + offset.z;

    // 4. the airflow.
    const float water = static_cast<IHeightmapCluster*>(heightmapCluster)
                            ->getWaterLevelInWorldCoords(at.x, at.z);                // +0x98
    Vec3 airflow;
    bool underWater;
    if (water <= at.y)
    {
        underWater = false;
        const Vec3& tangent = getTangentSpeed(at);                                   // +0xA4
        const Vec3 wind = static_cast<IPhysicsSystem*>(basicPhysicsSystem)->getWind();   // +0x28
        airflow.x = tangent.x - wind.x;
        airflow.y = tangent.y - wind.y;
        airflow.z = tangent.z - wind.z;
    }
    else
    {
        underWater = true;
        airflow = getTangentSpeed(at);
    }

    // 5. the lift, along the wing's own local Y (the +0x23C matrix's second row).
    Vec3 wingUp;
    wingUp.x = m_matrix.m[4];
    wingUp.y = m_matrix.m[5];
    wingUp.z = m_matrix.m[6];

    Vec3 wingForward;                                                                // never read by the callee
    wingForward.x = m_matrix.m[8];
    wingForward.y = m_matrix.m[9];
    wingForward.z = m_matrix.m[10];

    const float lift = calculateLift(airflow, wingForward, wingUp, scale);

    float f;
    if (underWater)
    {
        f = lift * *g_phyWingWaterMod;                                               // ds:0x8AB0870
    }
    else
    {
        float altitude = at.y / static_cast<IPhysicsSystem*>(basicPhysicsSystem)
                                    ->getAirDensityZeroAtHeight();                   // +0x30
        if (altitude > 1.0f) altitude = 1.0f;
        if (altitude < 0.0f) altitude = 0.0f;
        f = lift * (1.0f - altitude);
    }
    if (f > 200.0f)  f = 200.0f;
    if (f < -200.0f) f = -200.0f;

    const float along = -f;
    Vec3 force;
    force.x = m_matrix.m[4] * along;
    force.y = m_matrix.m[5] * along;
    force.z = m_matrix.m[6] * along;
    getTopFixParent()->addAccelerationAtAbsolutePosition(at, force);                 // +0x90

    // 6. the regulator.
    if (!m_template->m_liftRegulated)                                                // template +0x22C
        return;

    const float speed = sqrtf(airflow.x * airflow.x + airflow.y * airflow.y + airflow.z * airflow.z);
    Vec3 along3;                                                                     // the TOP parent's third row
    along3.x = tm[8]  * speed;
    along3.y = tm[9]  * speed;
    along3.z = tm[10] * speed;

    const float regulatedLift = calculateLift(along3, wingForward, wingUp, scale);
    const float neutral = static_cast<Wing*>(m_object)->calculateNeutralLift(
                              along3, -m_template->m_pitchOffset, scale);            // template +0x21C

    // RAW-verified - see the header block. neutral x ratio, then plus the earlier lift.
    float value = -(neutral * m_template->m_wingToRegulatorRatio                     // template +0x234
                    + regulatedLift
                    + m_template->m_regulateToLift);                                 // template +0x230
    value = value * static_cast<IPhysicsSystem*>(basicPhysicsSystem)->getGravity() / -9.8199997f;
    if (value > 1.0f)  value = 1.0f;
    if (value < -1.0f) value = -1.0f;
    static_cast<Wing*>(m_object)->m_liftRegulator = value;                           // object +0x1B4
}

} // namespace world
} // namespace hfe
} // namespace dice
