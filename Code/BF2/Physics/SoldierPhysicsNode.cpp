// Path:     [INFERRED] Physics/SoldierPhysicsNode.cpp - see the header.

#include "SoldierPhysicsNode.h"

#include <math.h>

#include "BasicPhysicsSystem.h"
#include "../Core/Ids.h"
#include "../Game/Utility/Vars.h"          // the three tuning vars updatePositionalPhysics reads
#include "../Game/Objects/IObject.h"
#include "../Game/Objects/ObjectLayoutAccess.h"
#include "../Game/Objects/Game/Soldier.h"   // updateLatencyState samples getPose() (+0x278)

namespace dice {
namespace hfe {
namespace world {

// .bss pointers filled by this TU's static initialiser, names and defaults read off the
// registration sites (0x83768D7..0x8376929). `phy-print` defaults to -1, which is what keeps
// updatePositionalPhysics's printDebug arm shut - and printDebug is an empty 13-byte body anyway.
float* const g_posDampVar      = Vars::getFloat("p-pos-damp", 0.99000001f);        // ds:0x8ABAC50
float* const g_posDampWaterVar = Vars::getFloat("p-pos-damp-water", 0.89999998f);  // ds:0x8ABAC54
int*   const g_phyPrintVar     = Vars::getInt("phy-print", -1);                    // ds:0x8ABAC58

typedef object_detail::LayoutAccess L;

static const float EPS = 0.000001f;

static inline void snapSmall(Vec3& v)
{
    if (fabsf(v.x) < EPS) v.x = 0.0f;
    if (fabsf(v.y) < EPS) v.y = 0.0f;
    if (fabsf(v.z) < EPS) v.z = 0.0f;
}

static inline void zero(Vec3& v) { v.x = v.y = v.z = 0.0f; }

// Advanced soldier drag carries three inlined copies of this projection in both retail ports.
// The axes are not assumed to be unit length: the projection divides by the squared axis length.
static inline Vec3 projectOntoDragAxis(const Vec3& value, const Vec3& axis)
{
    const float axisLengthSquared = axis.x * axis.x + axis.y * axis.y + axis.z * axis.z;
    if (fabsf(axisLengthSquared) > 0.00000011920929f)
    {
        const float scale = (value.x * axis.x + value.y * axis.y + value.z * axis.z)
                          / axisLengthSquared;
        Vec3 projected = { axis.x * scale, axis.y * scale, axis.z * scale };
        return projected;
    }
    return Vec3::getZero();
}

// Preserve the ordered branches from the x87 code, including their NaN behavior. Soldier drag
// passes an already-negative force into this helper and adds the result without negating it.
static inline float clipSoldierDragComponent(float speed, float drag)
{
    if (speed >= 0.0f)
    {
        if (speed > drag)
            return drag;
    }
    else if (speed <= drag)
    {
        return drag;
    }
    return speed;
}

static inline IPhysicsSystem* physicsSystem()
{
    return static_cast<IPhysicsSystem*>(basicPhysicsSystem);
}

// PointPhysicsNode ctor, then the fields; otherCollisionLod = 2.
SoldierPhysicsNode::SoldierPhysicsNode()
{
    m_refCount = 1;
    zero(m_rotationalSpeed);
    m_positionalFriction = m_positionalAcceleration;     // the base just zeroed it - (0,0,0)
    m_frictionCount = 0;
    zero(m_collidingMeshTangentSpeed);
    m_tangentSpeedTtl = 0;
    m_prevTransformation = Mat4::getIdentity();
    m_prevTransformationIsObjects = true;
    m_latency = 0;
    m_latencyIndex = 0;
    m_otherCollisionLod = 2;                         // +0x8C
}


SoldierPhysicsNode::~SoldierPhysicsNode()
{
    if (m_latency)
        delete[] m_latency;
}

void SoldierPhysicsNode::addRef()  { ++m_refCount; }
int  SoldierPhysicsNode::getRef()  { return m_refCount; }
int  SoldierPhysicsNode::release()
{
    const int r = --m_refCount;
    if (r == 0) delete this;
    return r;
}

void* SoldierPhysicsNode::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase)         return const_cast<SoldierPhysicsNode*>(this);
    if (iid == IID_IPhysicsNode)  return const_cast<SoldierPhysicsNode*>(this);
    return 0;
}

IPhysicsNode* SoldierPhysicsNode::getParent() const
{
    IObject* p = L::parent(m_object);
    return p ? static_cast<IPhysicsNode*>(L::physicsNode(p)) : 0;
}

void SoldierPhysicsNode::setAbsoluteTransformation(const Mat4& m) { m_object->setAbsoluteTransformation(m); }
void SoldierPhysicsNode::setAbsolutePosition(const Vec3& p)       { m_object->setAbsolutePosition(p); }

const Mat4& SoldierPhysicsNode::getPrevTransformation() const
{
    if (m_prevTransformationIsObjects)
        return m_object->getAbsoluteTransformation();
    return m_prevTransformation;
}

void SoldierPhysicsNode::setPrevTransformation(const Mat4& m)
{
    m_prevTransformationIsObjects = false;
    m_prevTransformation = m;
}

const Vec3& SoldierPhysicsNode::getPositionalSpeed() const
{
    if (L::parent(m_object))
        return Vec3::getZero();
    return m_positionalSpeed;
}

const Vec3& SoldierPhysicsNode::getRotationalSpeed() const
{
    if (L::parent(m_object))
        return Vec3::getZero();
    return m_rotationalSpeed;
}

// / 0x8376A60, 143 B: Y only, then the snap on all three.
void SoldierPhysicsNode::setRotationalSpeed(const Vec3& v)
{
    m_rotationalSpeed.y = v.y;
    snapSmall(m_rotationalSpeed);
}

void SoldierPhysicsNode::addRotationalSpeed(const Vec3& v)
{
    m_rotationalSpeed.y = v.y + m_rotationalSpeed.y;
    snapSmall(m_rotationalSpeed);
}

void SoldierPhysicsNode::setPositionalFriction(const Vec3& v)
{
    m_positionalFriction = v;
    snapSmall(m_positionalFriction);
}

// the running mean.
void SoldierPhysicsNode::addFrictionAtAbsolutePosition(const Vec3&, const Vec3& f)
{
    if (!m_enabled)
        return;
    const float n = static_cast<float>(m_frictionCount);
    const float sz = m_positionalFriction.z * n + f.z;
    const float sx = m_positionalFriction.x * n + f.x;
    const float sy = m_positionalFriction.y * n + f.y;
    const float inv = 1.0f / static_cast<float>(m_frictionCount + 1);
    m_positionalFriction.x = sx * inv;
    m_positionalFriction.y = sy * inv;
    m_positionalFriction.z = sz * inv;
    ++m_frictionCount;
}

// / : this class's simple positional drag. Like the
// RotationalPointPhysicsNode twin, it writes straight into the accumulator: there is no virtual
// acceleration call and no epsilon snap. Both retail ports preserve the component-wise arithmetic.
void SoldierPhysicsNode::updatePositionalDragSimple(float radius, float dragMod)
{
    if (!m_enabled)
        return;
    const Vec3 wind = physicsSystem()->getWind();
    const float rx = m_positionalSpeed.x * dragMod - wind.x;
    const float ry = m_positionalSpeed.y * dragMod - wind.y;
    const float rz = dragMod * m_positionalSpeed.z - wind.z;
    const float ax = rx * 3.1415927f * radius * radius * m_drag;
    const float ay = ry * 3.1415927f * radius * radius * m_drag;
    const float az = radius * (3.1415927f * rz * radius) * m_drag;
    const float invMass = 1.0f / m_mass;
    m_positionalAcceleration.x = m_positionalAcceleration.x - ax * invMass;
    m_positionalAcceleration.y = m_positionalAcceleration.y - ay * invMass;
    m_positionalAcceleration.z = m_positionalAcceleration.z - az * invMass;
}

// updatePositionalDragAdvanced - / This is the
// soldier-specific twin of PhysicsNode's rigid-body drag routine, not a call through to it. It
// projects wind-relative velocity onto the object's three (possibly scaled) axes, weights those
// projections by the fixed soldier cross sections supplied by updatePhysics, and adds the result
// directly to the point node's positional acceleration. There is no platform guard, collision-box
// force point, or torque arm on this path. Both retail ports agree on the ordering below.

// The negative sign is deliberately folded into dragScale before clipping. Consequently this
// routine's clipping is observably different from PhysicsNode's positive-scale-then-negate path.
void SoldierPhysicsNode::updatePositionalDragAdvanced(float areaZ, float areaY, float areaX,
                                                       float velocityFactor)
{
    if (!m_enabled)
        return;

    const Mat4& transformation = getAbsoluteTransformation();
    const Vec3 wind = physicsSystem()->getWind();
    const Vec3 relativeVelocity = {
        m_positionalSpeed.x * velocityFactor - wind.x,
        m_positionalSpeed.y * velocityFactor - wind.y,
        m_positionalSpeed.z * velocityFactor - wind.z
    };
    const float dragScale = sqrtf(relativeVelocity.x * relativeVelocity.x
                                + relativeVelocity.y * relativeVelocity.y
                                + relativeVelocity.z * relativeVelocity.z)
                          * -m_drag / m_mass;

    const Vec3 axisX = { transformation.m[0], transformation.m[1], transformation.m[2] };
    const Vec3 axisY = { transformation.m[4], transformation.m[5], transformation.m[6] };
    const Vec3 axisZ = { transformation.m[8], transformation.m[9], transformation.m[10] };
    const Vec3 projectedX = projectOntoDragAxis(relativeVelocity, axisX);
    const Vec3 projectedY = projectOntoDragAxis(relativeVelocity, axisY);
    const Vec3 projectedZ = projectOntoDragAxis(relativeVelocity, axisZ);

    Vec3 drag = {
        projectedX.x * (dragScale * areaX)
            + projectedY.x * (dragScale * areaY)
            + projectedZ.x * (dragScale * areaZ),
        projectedX.y * (dragScale * areaX)
            + projectedY.y * (dragScale * areaY)
            + projectedZ.y * (dragScale * areaZ),
        projectedX.z * (dragScale * areaX)
            + projectedY.z * (dragScale * areaY)
            + projectedZ.z * (dragScale * areaZ)
    };
    drag.x *= 0.033333335f;
    drag.y *= 0.033333335f;
    drag.z *= 0.033333335f;
    drag.x = clipSoldierDragComponent(m_positionalSpeed.x, drag.x) * 30.0f;
    drag.y = clipSoldierDragComponent(m_positionalSpeed.y, drag.y) * 30.0f;
    drag.z = clipSoldierDragComponent(m_positionalSpeed.z, drag.z) * 30.0f;

    m_positionalAcceleration.x += drag.x;
    m_positionalAcceleration.y += drag.y;
    m_positionalAcceleration.z += drag.z;
    snapSmall(m_positionalAcceleration);
}

const Vec3& SoldierPhysicsNode::getTangentSpeed(const Vec3&)
{
    return getRootNode()->getPositionalSpeed();
}

const Vec3& SoldierPhysicsNode::getPositionalFriction() const { return m_positionalFriction; }

// Integrates soldier motion for dt; this nonvirtual method hides PointPhysicsNode's version.
// Clamps acceleration above 1000 but discards friction above 250. Submersion above 0.05 uses at
// least a half-water damping blend. Moves by mean pre/post velocity and clears all three accumulators.
void SoldierPhysicsNode::updatePositionalPhysics(float dt)
{
    if (!m_enabled)
        return;

    const float accelerationSq = m_positionalAcceleration.x * m_positionalAcceleration.x
                               + m_positionalAcceleration.y * m_positionalAcceleration.y
                               + m_positionalAcceleration.z * m_positionalAcceleration.z;
    if (accelerationSq > 1000000.0f)                             // length > 1000
    {
        const float scale = 1000.0f / sqrtf(accelerationSq);
        m_positionalAcceleration.x *= scale;
        m_positionalAcceleration.y *= scale;
        m_positionalAcceleration.z *= scale;
    }

    if (m_positionalFriction.x * m_positionalFriction.x
        + m_positionalFriction.y * m_positionalFriction.y
        + m_positionalFriction.z * m_positionalFriction.z > 62500.0f)   // length > 250
    {
        m_positionalFriction.x = 0.0f;                            // discarded, not clamped
        m_positionalFriction.y = 0.0f;
        m_positionalFriction.z = 0.0f;
    }

    if (*g_phyPrintVar != -1)
        printDebug();

    addPositionalSpeed(m_localLinearSpeed);                       // the impulses folded in first

    const Vec3 speedBefore = m_positionalSpeed;

    Vec3 step;
    step.x = (m_positionalAcceleration.x + m_positionalFriction.x) * dt;
    step.y = (m_positionalAcceleration.y + m_positionalFriction.y) * dt;
    step.z = (m_positionalAcceleration.z + m_positionalFriction.z) * dt;
    addPositionalSpeed(step);

    float underWater = m_underWater;
    if (underWater >= 1.0f)
        underWater = 1.0f;
    if (underWater > 0.050000001f && underWater < 0.5f)
        underWater = 0.5f;                                        // the snap - see above
    const float damping = (1.0f - underWater) * *g_posDampVar + underWater * *g_posDampWaterVar;
    m_positionalSpeed.x *= damping;
    m_positionalSpeed.y *= damping;
    m_positionalSpeed.z *= damping;

    m_linearIntegrationVelocity.x = (m_positionalSpeed.x + speedBefore.x) * 0.5f;
    m_linearIntegrationVelocity.y = (m_positionalSpeed.y + speedBefore.y) * 0.5f;
    m_linearIntegrationVelocity.z = (m_positionalSpeed.z + speedBefore.z) * 0.5f;

    const Vec3& here = getAbsolutePosition();                     // -> the object, slot 27
    Vec3 there;
    there.x = here.x + dt * m_linearIntegrationVelocity.x;
    there.y = here.y + dt * m_linearIntegrationVelocity.y;
    there.z = here.z + dt * m_linearIntegrationVelocity.z;
    setAbsolutePosition(there);                                   // -> the object, slot 28

    m_positionalAcceleration.x = 0.0f;
    m_positionalAcceleration.y = 0.0f;
    m_positionalAcceleration.z = 0.0f;
    m_positionalFriction.x = 0.0f;
    m_positionalFriction.y = 0.0f;
    m_positionalFriction.z = 0.0f;
    m_localLinearSpeed.x = 0.0f;
    m_localLinearSpeed.y = 0.0f;
    m_localLinearSpeed.z = 0.0f;
}

// see the header. The dropped getRootNode() and the second getDragMod are the
// ELF's dead calls; the first is kept (a virtual call), the second - a pure function whose result
// is discarded - is not.
void SoldierPhysicsNode::updatePhysics(float dt)
{
    if (!m_enabled)
        return;
    getRootNode();
    if (m_drag > 0.0f)
    {
        float u = 1.0f;
        if (m_underWater < 1.0f)
            u = m_underWater;
        const float dragMod = getDragMod(u);
        updatePositionalDragAdvanced(0.25f, 0.25f, 0.89999998f, dragMod);
    }
    updatePositionalPhysics(dt);
    const float g = physicsSystem()->getGravity();
    m_frictionCount = 0;
    m_positionalAcceleration.y = g * m_gravityModifier + m_positionalAcceleration.y;
}

void SoldierPhysicsNode::setHasSeparatePhysicsUpdate(bool v) { m_hasSeparatePhysicsUpdate = v; }

// see the header.
void SoldierPhysicsNode::reset(bool full)
{
    PointPhysicsNode::reset(full);
    zero(m_positionalFriction);
    m_frictionCount = 0;
    zero(m_rotationalSpeed);
    zero(m_collidingMeshTangentSpeed);
    m_tangentSpeedTtl = 0;
    if (full)
    {
        m_prevTransformationIsObjects = true;
        m_latencyIndex = 0;
        if (m_latency)
        {
            for (int i = 0; i < 16; ++i)
            {
                m_latency[i].position = Vec3::getZero();
                m_latency[i].rotation.w = 1.0f;
                m_latency[i].rotation.x = 0.0f;
                m_latency[i].rotation.y = 0.0f;
                m_latency[i].rotation.z = 0.0f;
                m_latency[i].pose = 0;                     // the ELF zeroes the whole dword here
                m_latency[i].pad[0] = m_latency[i].pad[1] = m_latency[i].pad[2] = 0;
            }
        }
    }
}

bool SoldierPhysicsNode::getIsSleeping() const { return false; }
void SoldierPhysicsNode::setIsSleeping()       {}
void SoldierPhysicsNode::setSleepiness(int)    {}
int  SoldierPhysicsNode::getSleepiness() const { return 1; }
void SoldierPhysicsNode::setIsSleepier()       {}
void SoldierPhysicsNode::setIsAwake()          { PointPhysicsNode::setIsAwake(); }
bool SoldierPhysicsNode::getIsPoint() const    { return false; }

// one ring of sixteen {0, (0,0,0,1), 0}.
void SoldierPhysicsNode::initLatencyCompensation()
{
    LatencyEntry* ring = new LatencyEntry[16];
    for (int i = 0; i < 16; ++i)
    {
        ring[i].position.x = ring[i].position.y = ring[i].position.z = 0.0f;
        ring[i].rotation.x = ring[i].rotation.y = ring[i].rotation.z = 0.0f;
        ring[i].rotation.w = 1.0f;
        ring[i].pose = 0;                                  // the ELF zeroes the whole dword here
        ring[i].pad[0] = ring[i].pad[1] = ring[i].pad[2] = 0;
    }
    m_latency = ring;
}

bool SoldierPhysicsNode::hasLatencyCompensation() { return m_latency != 0; }

// Stores position, rotation and a one-byte pose in the 16-entry latency ring.
// m_object must be a Soldier; samples use interleaved 32-byte records for pose-aware rewind.
void SoldierPhysicsNode::updateLatencyState()
{
    if (!m_latency)                                                        // +0x104
        return;

    const Mat4& transform = getAbsoluteTransformation();                   // vslot +0x3C

    LatencyEntry& entry = m_latency[m_latencyIndex];                       // +0x108, entries of 32 B
    entry.position.x = transform.m[12];
    entry.position.y = transform.m[13];
    entry.position.z = transform.m[14];

    Quat rotation;
    rotation.x = 0.0f;
    rotation.y = 0.0f;
    rotation.z = 0.0f;                                                     // dead, as compiled
    rotation.fromMat(transform);
    entry.rotation = rotation;

    entry.pose = static_cast<unsigned char>(
        static_cast<Soldier*>(m_object)->getPose());                       // Soldier +0x278

    ++m_latencyIndex;
    if (m_latencyIndex == 16)
        m_latencyIndex = 0;
}

// Returns an interpolated historical transform from 1/30-second samples, capped at fourteen steps.
// Optional pose comes from the older sample. The returned historical matrix is shared static storage;
// noncompensated calls return the live transform and pose -1. Not reentrant.
const Mat4& SoldierPhysicsNode::getLatencyCompensatedTransformation(float latency, int* pose)
{
    const float samplePeriod = 0.033333335f;
    if (!m_enabled || !m_latency || latency <= samplePeriod)
    {
        if (pose)
            *pose = -1;
        return getAbsoluteTransformation();
    }

    float integralSamples = 0.0f;
    const float fractionalSamples = modff(latency / samplePeriod, &integralSamples);
    const float blend = 1.0f - fractionalSamples;

    int sampleCount = static_cast<int>(integralSamples);
    if (sampleCount >= 15)
        sampleCount = 14;

    int newerIndex = m_latencyIndex - sampleCount;
    if (newerIndex < 0)
        newerIndex += 16;
    if (newerIndex >= 16)
        newerIndex = 15;

    int olderIndex = newerIndex - 1;
    if (olderIndex < 0)
        olderIndex = 15;

    Quat rotation;
    rotation.x = 0.0f;
    rotation.y = 0.0f;
    rotation.z = 0.0f;
    rotation.w = 1.0f;
    rotation.slerp(m_latency[olderIndex].rotation, m_latency[newerIndex].rotation, blend);

    static Mat4 compensated = Mat4::getIdentity();
    rotation.toMat(compensated);
    compensated.m[12] = m_latency[olderIndex].position.x
        + (m_latency[newerIndex].position.x - m_latency[olderIndex].position.x) * blend;
    compensated.m[13] = m_latency[olderIndex].position.y
        + (m_latency[newerIndex].position.y - m_latency[olderIndex].position.y) * blend;
    compensated.m[14] = m_latency[olderIndex].position.z
        + (m_latency[newerIndex].position.z - m_latency[olderIndex].position.z) * blend;

    if (pose)
        *pose = m_latency[olderIndex].pose;
    return compensated;
}

IPhysicsNode* SoldierPhysicsNode::getRootNode()
{
    return static_cast<IPhysicsNode*>(L::physicsNode(getRootParent(m_object)));
}


void SoldierPhysicsNode::setCollidingMeshTangentSpeed(const Vec3& v)
{
    m_collidingMeshTangentSpeed = v;
    m_tangentSpeedTtl = 20;
}

void SoldierPhysicsNode::getCollidingMeshTangentSpeed(Vec3& out)
{
    if (m_tangentSpeedTtl <= 0)
    {
        out.x = out.y = out.z = 0.0f;
        return;
    }
    --m_tangentSpeedTtl;
    out = m_collidingMeshTangentSpeed;
}

void SoldierPhysicsNode::printDebug() const {}

float SoldierPhysicsNode::getDragMod(float u)
{
    return u * 25.0f + 1.0f - u;
}

} // namespace world
} // namespace hfe
} // namespace dice
