// Soldier collision and pose-change response. Physics nodes integrate motion; responses resolve contact.
// Soldier tests dispatch by partner kind and validate pose changes against surrounding geometry.
// ResponsePhysicsManager brackets each fixed substep with startFrame/stopFrame and finishes the
// render frame with postFrameUpdate.

#include "SoldierResponse.h"

#include <float.h>
#include <math.h>
#include <cstdio>      // the [TEMP] stuck-soldier tracer
#include <cstdarg>     // ditto
#include <cstdlib>
#include <vector>

#include "../Animation/BoneAnimation/Skeleton.h"       // projectile collision rig query
#include "../Core/Debug.h"                            // getSoldierHeight's attested line 597
#include "../Core/SmartItf.h"                         // existing hfe::classManager declaration
#include "../Core/Ids.h"
#include "../Game/IGameLogic.h"                       // gameLogic->giveDamage - the two crush arms
#include "../Game/Objects/IObject.h"
#include "../Game/Objects/IPlayer.h"                  // projectile owner's vehicle
#include "../Game/Objects/Game/Soldier.h"             // m_object is a Soldier here - see the header
#include "../Game/Objects/Projectiles/GenericProjectile.h" // typed man-down collision policy
#include "../Game/Objects/GameLogicComp/IArmorGLComp.h"// projectile hit record
#include "../Game/Objects/ObjectLayoutAccess.h"
#include "../Game/Utility/Vars.h"
#include "../IO/Network/INetworkable.h"               // solveImpulse's remote-ghost gate
#include "../Math/Mat4.h"
#include "../Scene/Object/ObjectManager.h"            // objectManager (0x8B1B920) + the grid query
#include "../Scene/Object/ObjectPredicator.h"         // ObjectIgnoreInterfacePredicator
#include "BasicPhysicsSystem.h"                       // IPhysicsSystem::getGravity - the buoyancy arm
#include "CollisionStats.h"
#include "HeightmapCluster.h"
#include "ICollisionMesh.h"
#include "IPhysicsNode.h"
#include "LatencyCompensation.h"
#include "MaterialManager.h"
#include "SoldierPhysicsNode.h"

namespace dice {
namespace hfe {
namespace world {

typedef object_detail::LayoutAccess L;

extern IBase* heightmapCluster;                       // ds:0x8ABAF24 - Core/Globals.cpp
extern IBase* materialManager;                        // ds:0x8AD2820 - Core/Globals.cpp
extern IBase* basicPhysicsSystem;                     // ds:0x8B2784C - Core/Globals.cpp

// world::setAdjustVector / world::addAdjustVector - Physics/ResponsePhysics.cpp (0x8365470 /
// 0x8365360). Shared by both response families; declared here rather than pulling in the whole
// ResponsePhysics header for two free functions.
void setAdjustVector(Vec3* accumulator, const Vec3* value);
void addAdjustVector(Vec3* accumulator, const Vec3* value, int count);

// Game/Objects/Projectiles/GenericProjectile.cpp. The projectile interface itself remains opaque
// here; this recovered seam returns its owning player without reproducing a raw vtable walk.
IPlayer* projectileOwner(void* projectile);

// The TU's cvars (static-init _390). Every handle read by a written body is registered here with the
// ELF's name and default. Handles that remain banked belong only to still-unwritten bodies.
static float* const g_feetContactNormal   = Vars::getFloat("phy-soldier-feet-contact-normal", 0.5f);   // ds:0x8ABAE00
static float* const g_extendRay           = Vars::getFloat("coll-soldier-extend-ray", 0.89999998f);     // ds:0x8ABADF4
static float* const g_feetLevel           = Vars::getFloat("phy-soldier-feet-level", -0.039999999f);    // ds:0x8ABADFC
static int*   const g_takeFallDamage      = Vars::getInt("phy-soldier-take-fall-damage", 1);           // ds:0x8ABAE10
static float* const g_overrunImpactMod    = Vars::getFloat("coll-soldier-overrun-impact-mod", 2.5f);   // ds:0x8ABAE14
static float* const g_crushDepth          = Vars::getFloat("coll-soldier-crush-depth", 0.050000001f);  // ds:0x8ABAE18
static float* const g_testRadiusMod       = Vars::getFloat("coll-soldier-collision-test-radius-mod", 0.99899995f);   // ds:0x8ABAE1C
static float* const g_collisionAdjustMod  = Vars::getFloat("coll-soldier-collision-adjust-mod", 2.0f); // ds:0x8ABAE20
static int*   const g_testCount           = Vars::getInt("coll-soldier-collision-test-count", 8);      // ds:0x8ABAE24
static int*   const g_testCountStartBackAt =
    Vars::getInt("coll-soldier-collision-test-count-start-backAt", 3);                                 // ds:0x8ABAE28
static float* const g_impMod              = Vars::getFloat("phy-imp-mod", 1.0f);                       // ds:0x8ABAE30
static int*   const g_drawSoldier         = Vars::getInt("coll-draw-soldier", 0);                       // ds:0x8ABAE34
static float* const g_crouchHeight        = Vars::getFloat("coll-soldier-crouch-height", 1.39999998f); // ds:0x8ABAE38
static float* const g_proneHeight         = Vars::getFloat("coll-soldier-prone-height", 0.80000001f);  // ds:0x8ABAE3C
static float* const g_pivotHeight         = Vars::getFloat("coll-soldier-pivot-height", 1.0f);         // ds:0x8ABAE40
static float* const g_startFloat          = Vars::getFloat("phy-soldier-start-float", 0.99000001f);    // ds:0x8ABAE48
static float* const g_stopFloat           = Vars::getFloat("phy-soldier-stop-float", 0.89999998f);     // ds:0x8ABAE4C
static float* const g_crushMinPosSpeedSq  = Vars::getFloat("phy-soldier-crush-minposspeedsqr", 12.0f); // ds:0x8ABAE50
static float* const g_crushMinVehicleSpeedSq =
    Vars::getFloat("phy-soldier-crush-minvehicleposspeedsqr", 2.0f);                                   // ds:0x8ABAE54

// ds:0x8ABAE68 - not a cvar: a plain .bss int that solveImpulse increments on entry and decrements
// on every exit. Only depth 1 runs the iteration, which is what makes the recursive solveImpulse
// call inside the loop degenerate into "commit the positional adjust and clear it".
static int g_solveImpulseDepth = 0;

// The tick the whole soldier collision stack is hard-coded to. 0x3D088889 appears as a literal at
// every call site (internal_checkVsTerrain, checkSoldierVsMesh, checkSoldierVsSoldier) - it is not
// the frame's deltaTime.
static const float k30HzTick = 0.033333335f;

static inline void zero(Vec3& v) { v.x = v.y = v.z = 0.0f; }

// IDebugRenderer::drawObject is still an unrecovered debug-only interface slot, +0x14 in both
// ports. Projectile policy dispatch below uses the recovered concrete type instead.
#if defined(_MSC_VER)
#  define BF2_SOLDIER_VCALL __thiscall
#else
#  define BF2_SOLDIER_VCALL
#endif

template <class Fn>
static inline Fn soldierVSlot(void* object, unsigned int byteOffset)
{
    return reinterpret_cast<Fn>((*reinterpret_cast<void***>(object))[byteOffset / 4]);
}

typedef void (BF2_SOLDIER_VCALL* DebugDrawObjectFn)(void*, unsigned int,
                                                     const Mat4&, const std::string&);
#undef BF2_SOLDIER_VCALL

static bool projectileDamagesMandownSoldiers(void* projectile)
{
    // ELF secondary table 0x88BC658 +0xC0 -> thunk 0x8287240 -> getter 0x82848B0.
    // PE secondary table 0x7DB830 +0xBC -> getter 0x5B11F0. The current IProjectileObject
    // declares only its destructor, so those retail offsets cannot be used on its rebuilt table.
    // GenericProjectile is the sole implementor; use its real virtual getter without adding a gate.
    return static_cast<GenericProjectile*>(static_cast<IProjectileObject*>(projectile))
        ->getDamageMandownSoldiers();
}

static void drawSoldierProbe(const Mat4& transform)
{
    // ELF 0x83797E6 loads hfe::classManager (0x8AC85A0), not a world-scoped global;
    // PE 0x6A30B7 loads the same engine singleton at 0x892D58. Renderer guard unchanged.
    IBase* const renderer = ::dice::hfe::classManager->getSingleton(std::string("DebugRenderer"));
    if (renderer)
        soldierVSlot<DebugDrawObjectFn>(renderer, 0x14)(
            renderer, geom::CID_DebugSphereMeshTemplate, transform, std::string());
}

static Vec3 transformPoint(const Mat4& transform, const Vec3& point)
{
    Vec3 result;
    result.x = transform.m[4] * point.y + transform.m[0] * point.x
             + transform.m[8] * point.z + transform.m[12];
    result.y = transform.m[9] * point.z + transform.m[5] * point.y
             + transform.m[1] * point.x + transform.m[13];
    result.z = point.z * transform.m[10] + point.y * transform.m[6]
             + point.x * transform.m[2] + transform.m[14];
    return result;
}

static Vec3 normalizedLikeEngine(const Vec3& value)
{
    Vec3 result = value;
    const float lengthSq = value.z * value.z + value.y * value.y + value.x * value.x;
    if (fabsf(lengthSq - 1.0f) >= FLT_EPSILON)
    {
        if (fabsf(lengthSq) >= FLT_EPSILON)
        {
            const float invLength = 1.0f / sqrtf(lengthSq);
            result.x *= invLength;
            result.y *= invLength;
            result.z *= invLength;
        }
        else
        {
            zero(result);
        }
    }
    return result;
}

namespace {

// / PE 0x6A1B10. This is deliberately not a generic segment/sphere
// intersection: retail rejects a ray that starts inside or on the sphere, rejects a tangent, and
// returns the direction opposite the ray as the fly-by normal rather than the sphere's radial.
bool checkRayEntersSphere(Vec3* outPosition, Vec3* outNormal,
                          const Vec3& start, const Vec3& end,
                          const Vec3& center, float radius)
{
    Vec3 fromCenter;
    fromCenter.x = start.x - center.x;
    fromCenter.y = start.y - center.y;
    fromCenter.z = start.z - center.z;
    const float c = fromCenter.x * fromCenter.x + fromCenter.y * fromCenter.y
                  + fromCenter.z * fromCenter.z - radius * radius;
    if (!(0.0f < c))
        return false;

    Vec3 direction;
    direction.x = end.x - start.x;
    direction.y = end.y - start.y;
    direction.z = end.z - start.z;
    const float b = 2.0f * (fromCenter.x * direction.x + fromCenter.y * direction.y
                            + fromCenter.z * direction.z);
    const float a = direction.x * direction.x + direction.y * direction.y
                  + direction.z * direction.z;
    const float discriminant = b * b - 4.0f * c * a;
    if (!(0.0f < discriminant))
        return false;

    const float root = sqrtf(discriminant);
    const float numerator = b >= 0.0f ? root - b : -root - b;
    const float fraction = numerator / (2.0f * a);
    if (fraction < 0.0f || !(fraction < 1.0f))
        return false;

    if (outPosition)
    {
        outPosition->x = start.x + direction.x * fraction;
        outPosition->y = start.y + direction.y * fraction;
        outPosition->z = start.z + direction.z * fraction;
    }
    if (outNormal)
    {
        Vec3 reverse;
        reverse.x = -direction.x;
        reverse.y = -direction.y;
        reverse.z = -direction.z;
        *outNormal = normalizedLikeEngine(reverse);
    }
    return true;
}

} // anonymous namespace

// PointResponsePhysics ctor, then every field.
SoldierResponsePhysics::SoldierResponsePhysics()
{
    zero(m_positionalAdjusts1);
    m_refCount = 1;
    zero(m_positionalAdjusts2);
    zero(m_surfacePositionalSpeed);
    zero(m_positionalAdjusts3);
    zero(m_feetContactNormal);
    m_contactCount = 0;
    m_averageFriction = 1.0f;
    m_collisionChecked2 = 0;
    m_averageElasticity = 0.0f;
    m_averageResistance = 0.0f;
    m_frictionFlags = 0;
    m_permanentGrip = 1;
    m_dwordE4 = 0;
    zero(m_intersectingTerrainPoint);
    m_isIntersectingTerrain = false;
    m_byteF9 = false;
    m_byteFA = false;
    m_isFloating = false;
}

SoldierResponsePhysics::~SoldierResponsePhysics() {}

void SoldierResponsePhysics::addRef()  { ++m_refCount; }
int  SoldierResponsePhysics::getRef()  { return m_refCount; }
int  SoldierResponsePhysics::release()
{
    const int r = --m_refCount;
    if (r == 0) delete this;
    return r;
}

void* SoldierResponsePhysics::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase)             return const_cast<SoldierResponsePhysics*>(this);
    if (iid == IID_IResponsePhysics)  return const_cast<SoldierResponsePhysics*>(this);
    return 0;
}

// the byte pair shifts, the feet normal is reloaded from its cvar.
void SoldierResponsePhysics::reset()
{
    zero(m_positionalAdjusts1);
    zero(m_positionalAdjusts2);
    const bool was = m_byteFA;
    m_contactCount = 0;
    m_byteFA = false;
    m_byteF9 = was;
    m_feetContactNormal.x = 0.0f;
    m_feetContactNormal.y = *g_feetContactNormal;
    m_feetContactNormal.z = 0.0f;
    zero(m_positionalAdjusts3);
}

unsigned int SoldierResponsePhysics::getClassID() const { return CID_SoldierResponsePhysics; }

// checkObjectVsObject - / The soldier-specific dispatcher.
// The trailing bool is not read. A disabled soldier ignores every mesh and soldier, but projectiles
// represented by either point-response class may still hit it when their IProjectileObject policy
// explicitly allows damage to a man-down soldier.
void SoldierResponsePhysics::checkObjectVsObject(IObject* soldier, IObject* other,
                                                  float dt, bool /*unused*/)
{
    IResponsePhysics* const response = static_cast<IResponsePhysics*>(L::responsePhysics(other));
    if (!response)
        return;

    const unsigned int cid = response->getClassID();
    if (!m_enabled)
    {
        if (cid != CID_PointResponsePhysics && cid != CID_RotationalPointResponsePhysics)
            return;
        if (!other)
            return;
        void* const projectile = other->queryInterface(IID_IProjectileObject);
        if (!projectile || !projectileDamagesMandownSoldiers(projectile))
            return;
        checkSoldierVsPoint(soldier, other, dt);
        return;
    }

    if (cid == CID_SoldierResponsePhysics)
    {
        checkSoldierVsSoldier(soldier, other, dt, 1.0f);
        return;
    }
    if (cid == CID_PointResponsePhysics || cid == CID_RotationalPointResponsePhysics)
    {
        checkSoldierVsPoint(soldier, other, dt);
        return;
    }
    if (cid == CID_ResponsePhysics || cid == CID_StaticResponsePhysics)
    {
        checkSoldierVsMesh(soldier, other, dt, 1.0f, true);
        return;
    }

    Debug(DEBUGTYPE_6, "Physics/SoldierResponse.cpp", 525)
        << "Unknown response physics class";
}

// The returned penetration is discarded here (`fstp st` at 0x8379F31) - only
// solveImpulse reads it.
void SoldierResponsePhysics::checkVsTerrain(float dt)
{
    internal_checkVsTerrain(dt, 1.0f);
}

// the tail call with the arguments shifted - first Vec3 dropped, bool false.
void SoldierResponsePhysics::impulseOn(const Vec3&, const Vec3& b, const Vec3& c, float f, int i, int j)
{
    if (!m_enabled)
        return;
    internalImpulseOn(b, c, f, i, j, false);
}

// Returns the deepest negative terrain/water penetration among five foot points, or zero.
// Ignores dt and uses scale. Queries collision LOD on the object and re-fetches the
// vertex vector for each point.
float SoldierResponsePhysics::internal_checkVsTerrain(float /*dt*/, float scale)
{
    if (!m_enabled)
        return 0.0f;
    if ((getCollisionGroups() & 1) != 0)                                  // +0x54
        return 0.0f;
    ++collisionStatsData->unknown1C;                                      // +0x1C - attested here
    if (!m_object->hasCollisionLod(1))                                    // slot 51, on the object
        return 0.0f;

    IObject* const object = m_object;
    ICollisionMesh* const mesh = static_cast<ICollisionMesh*>(L::collisionMesh(object));   // +0x40
    IPhysicsNode* const node   = static_cast<IPhysicsNode*>(L::physicsNode(object));       // +0x44
    IHeightmapCluster* const cluster = static_cast<IHeightmapCluster*>(heightmapCluster);

    bool intersecting = false;
    Vec3 normal;
    zero(normal);
    Vec3 point;
    zero(point);
    Vec3 world;
    zero(world);

    const Vec3& position = object->getAbsolutePosition();                 // slot 27
    float lowestY = 3.4028235e38f;
    float deepest = 0.0f;
    const float* const m = object->getAbsoluteTransformation().m;         // slot 33

    Vec3 adjustMean;
    zero(adjustMean);
    int contactCount = 0;

    for (int i = 0; i < 5; ++i)                                           // five, not getVertexCount
    {
        Vec3 rotated;
        zero(rotated);
        const float* const v =
            static_cast<const float*>(mesh->getVertexVector(1)) + i * 3;  // slot 17
        Vec3 scaled;
        scaled.x = scale * v[0];
        scaled.y = scale * v[1];
        scaled.z = scale * v[2];
        rotated.x = m[4] * scaled.y + m[0] * scaled.x + m[8] * scaled.z;
        rotated.y = m[9] * scaled.z + m[5] * scaled.y + m[1] * scaled.x;
        rotated.z = scaled.z * m[10] + scaled.y * m[6] + scaled.x * m[2];
        world.x = rotated.x + position.x;
        world.y = rotated.y + position.y;
        world.z = rotated.z + position.z;

        Vec3 relative;
        zero(relative);
        Vec3 speed;
        zero(speed);

        unsigned char surfaceMaterial = 0;
        const float surface =
            cluster->getHeightAndNormalInWorldCoords(world.x, world.z, normal, surfaceMaterial);   // slot 33
        float depth = world.y - surface;
        if (!(0.0f < depth))
        {
            // the running minimum takes the unscaled penetration - the scaling below is for the
            // impulse only, and this is the value the function returns.
            if (depth <= deepest)
                deepest = depth;
            if (scale < 1.0f)
                depth = *g_collisionAdjustMod / scale * depth;

            point.x = world.x;
            point.y = surface;
            point.z = world.z;
            intersecting = true;

            relative.x = point.x - position.x;
            relative.y = point.y - position.y;
            relative.z = point.z - position.z;

            speed = node->getTangentSpeed(point);                         // +0xA4
            const int meshMaterial = mesh->getMaterialForVertex(1, i) & 0xFFFF;   // slot 18, a u16
            if (speed.x * speed.x + speed.y * speed.y + speed.z * speed.z > 0.1f
                && *g_takeFallDamage)
                object->handleCollision(0, speed, normal, relative, meshMaterial,
                                        surfaceMaterial, 0, 0);           // slot 45

            internalImpulseOn(speed, normal, depth * normal.y, meshMaterial, surfaceMaterial,
                              i == 0);

            // only point 0 - the foot - may claim the ground normal, and only if it is more
            // upright than what reset() floored the field at this frame.
            if (i == 0 && normal.y > m_feetContactNormal.y)
            {
                m_feetContactNormal.x = normal.x;
                m_feetContactNormal.y = normal.y;
                m_feetContactNormal.z = normal.z;
                m_byteFA = true;
            }

            addAdjustVector(&adjustMean, &point, contactCount);
            ++contactCount;
        }
        if (lowestY > world.y)
            lowestY = world.y;
    }

    // The water half runs only for a full-scale test; solveImpulse's 0.999 probe stops here.
    if (scale < 1.0f)
        return deepest;

    const float water = cluster->getWaterLevelInWorldCoords(position.x, position.z);   // slot 38
    const float aboveWater = lowestY - water;
    if (aboveWater >= 0.0f)
    {
        node->setUnderWater(0.0f);                                        // +0x108
        m_isFloating = false;
    }
    else
    {
        if (node->getUnderWater() <= 0.0f)                                // +0x10C - entering
        {
            const int firstMaterial = mesh->getMaterialForVertex(1, 0) & 0xFFFF;
            Vec3 up;
            up.x = 0.0f;
            up.y = 1.0f;
            up.z = 0.0f;
            Vec3 penetration;
            penetration.x = 0.0f;
            penetration.y = water - position.y;
            penetration.z = 0.0f;
            object->handleCollision(0, node->getPositionalSpeed(), up, penetration,
                                    firstMaterial, 1, 0, 0);              // +0x60, slot 45
        }
        node->setUnderWater(-aboveWater);

        if (aboveWater < -0.30000001f)
        {
            float factor = aboveWater;
            if (aboveWater > -1.0f)
            {
                float sq = aboveWater * aboveWater;
                if (sq >= 1.0f)
                    sq = 1.0f;
                factor = -sq;
            }
            const float g =
                static_cast<IPhysicsSystem*>(basicPhysicsSystem)->getGravity() * factor;   // +0x20
            Vec3 acceleration;
            acceleration.x = 0.0f;
            acceleration.y = node->getGravityModifier() * g;              // +0xFC
            acceleration.z = 0.0f;
            node->addAccelerationAtRelativePosition(Vec3::getZero(), acceleration);   // +0x94
        }

        // shipped vestige: the height is queried and thrown away (0x837A5F8, `fstp st`).
        cluster->getHeightInWorldCoords(position.x, position.z);          // slot 27

        // the Schmitt trigger - sets at start-float, clears only below stop-float
        if (-aboveWater >= *g_startFloat)
            m_isFloating = true;
        else if (*g_stopFloat > -aboveWater)
            m_isFloating = false;

        if (m_isFloating)
        {
            m_feetContactNormal.x = 0.0f;
            m_feetContactNormal.y = 1.0f;
            m_feetContactNormal.z = 0.0f;
            m_averageFriction = 1.0f;
            m_positionalAdjusts3 = node->getTangentSpeed(point);          // +0xA4, the last point
        }
    }

    m_isIntersectingTerrain = intersecting;
    m_intersectingTerrainPoint = adjustMean;
    return deepest;
}

// Accumulates soldier impulse adjustments and material means without a contact list or effect record.
// isFirstPoint is unused; the original redundant zero initializations are omitted.
void SoldierResponsePhysics::internalImpulseOn(const Vec3& speed, const Vec3& normal, float depth,
                                               int ownMaterial, int otherMaterial, bool /*isFirstPoint*/)
{
    if (!m_enabled)
        return;

    MaterialManager* const materials = static_cast<MaterialManager*>(materialManager);

    const float negX = -speed.x;
    const float negY = -speed.y;
    const float negZ = -speed.z;

    // the normal component of -speed
    Vec3 speedAdjust;
    const float nn = normal.y * normal.y + normal.x * normal.x + normal.z * normal.z;
    if (fabsf(nn) > FLT_EPSILON)
    {
        const float k = (negY * normal.y + negX * normal.x + negZ * normal.z) / nn;
        speedAdjust.x = normal.x * k;
        speedAdjust.y = normal.y * k;
        speedAdjust.z = k * normal.z;
    }
    else
    {
        speedAdjust.x = speedAdjust.y = speedAdjust.z = 0.0f;
    }

    Vec3 positionalAdjust;
    positionalAdjust.x = normal.x * -depth;
    positionalAdjust.y = normal.y * -depth;
    positionalAdjust.z = normal.z * -depth;

    setAdjustVector(&m_positionalAdjusts1, &positionalAdjust);
    setAdjustVector(&m_positionalAdjusts2, &speedAdjust);
    addAdjustVector(&m_positionalAdjusts3, &speed, m_contactCount);
    ++m_contactCount;

    m_frictionFlags = static_cast<unsigned char>(getPermanentGrip() | (m_frictionFlags & 0x40));

    if (ownMaterial)
    {
        if (otherMaterial == 7001)
        {
            m_averageFriction = 0.0f;                    // the one frictionless material
        }
        else
        {
            const float f0 =
                materials->getFrictionForMaterial(static_cast<unsigned int>(ownMaterial));     // +0x6C
            m_averageFriction =
                (materials->getFrictionForMaterial(static_cast<unsigned int>(otherMaterial)) + f0) * 0.5f;
        }
        const float e0 =
            materials->getElasticityForMaterial(static_cast<unsigned int>(ownMaterial));       // +0x70
        m_averageElasticity =
            (materials->getElasticityForMaterial(static_cast<unsigned int>(otherMaterial)) + e0) * 0.5f;
        const float r0 =
            materials->getResistanceForMaterial(static_cast<unsigned int>(ownMaterial));       // +0x74
        m_averageResistance =
            (materials->getResistanceForMaterial(static_cast<unsigned int>(otherMaterial)) + r0) * 0.5f;
    }
}

void SoldierResponsePhysics::setCollisionChecked(unsigned int v)     { m_collisionChecked2 = v; }
unsigned int SoldierResponsePhysics::getCollisionChecked() const     { return m_collisionChecked2; }

bool SoldierResponsePhysics::getIsIntersectingTerrain(Vec3& out) const
{
    out = m_intersectingTerrainPoint;
    return m_isIntersectingTerrain;
}

unsigned char SoldierResponsePhysics::getPermanentGrip() const       { return m_permanentGrip; }
void SoldierResponsePhysics::setPermanentGrip(unsigned char v)       { m_permanentGrip = v; }
void SoldierResponsePhysics::setSurfacePositionalSpeed(const Vec3& v){ m_surfacePositionalSpeed = v; }
const Vec3& SoldierResponsePhysics::getSurfacePositionalSpeed() const{ return m_surfacePositionalSpeed; }
const Vec3& SoldierResponsePhysics::getPositionalAdjusts() const     { return m_positionalAdjusts1; }

// poseChangeTest - / PE 0x6A4450. This tests only expansions: standing to
// crouch is an unconditional shrink, prone to crouch is checked, standing is always checked, and
// prone is accepted without a query. Pose 3 (swimming) always refuses a pose change. The function
// changes only its local pose value; its result tells Soldier whether the requested pose fits.
bool SoldierResponsePhysics::poseChangeTest(IObject* soldierObject, IObject* otherObject, float dt)
{
    if (!m_enabled)
        return true;

    Soldier* const soldier = static_cast<Soldier*>(soldierObject);
    SoldierPose pose = soldier->getPose();
    if (pose == SoldierPose_Swim)
        return false;

    Soldier* const responseSoldier = static_cast<Soldier*>(m_object);
    const float radius = responseSoldier->getSoldierRadius();
    const float standingHeight = responseSoldier->getStandingHeight();
    const float pivotHeight = *g_pivotHeight;
    const SoldierPose requestedPose = soldier->getRequestedPose();

    if (requestedPose == SoldierPose_Crouch)
    {
        if (pose != SoldierPose_Stand)
        {
            Vec3 bottom = soldierObject->getAbsolutePosition();
            bottom.y -= pivotHeight;
            Vec3 top = bottom;
            top.y += *g_crouchHeight - radius;
            if (!expandSoldierCollision(bottom, top, radius, otherObject, dt))
                return pose == requestedPose;
        }
        pose = SoldierPose_Crouch;
    }
    else if (requestedPose == SoldierPose_Stand)
    {
        Vec3 bottom = soldierObject->getAbsolutePosition();
        bottom.y -= pivotHeight;
        Vec3 top = bottom;
        top.y += standingHeight - radius;
        if (!expandSoldierCollision(bottom, top, radius, otherObject, dt))
            return pose == requestedPose;
        pose = SoldierPose_Stand;
    }
    else if (requestedPose == SoldierPose_Prone)
    {
        pose = SoldierPose_Prone;
    }
    else
    {
        Debug(DEBUGTYPE_6, "Physics/SoldierResponse.cpp", 1585)
            << "Requested pose does not exist";
        return pose == requestedPose;
    }

    return pose == requestedPose;
}

// testIfValidTransformation - / PE 0x6A3710. The object-manager temp
// vector is copied before the radius query (not cleared), and all per-sample results accumulate in
// four vectors across every response in every candidate. The bool changes only predicate bit 3.
// The filter is ObjectIgnoreInterfacePredicator: projectiles are excluded, ordinary map/static
// objects are retained. A single sphere/mesh hit makes the proposed soldier transformation invalid.
bool SoldierResponsePhysics::testIfValidTransformation(const Mat4& transformation,
                                                        bool includeNoCollisionObjects)
{
    std::vector<IObject*> objects(objectManager->getObjectTempVector());
    ObjectIgnoreInterfacePredicator predicator(
        0x1000100u + (includeNoCollisionObjects ? 8u : 0u), IID_IProjectileObject);

    Soldier* const soldier = static_cast<Soldier*>(m_object);
    const float radius = soldier->getSoldierRadius();
    const Vec3& center = *reinterpret_cast<const Vec3*>(&transformation.m[12]);
    objectManager->getObjectsFromGrid(
        center, m_object->getPhysicBoundingRadius(), objects, &predicator);
    const Vec3& zeroVelocity = Vec3::getZero();

    std::vector<Vec3> normals;
    std::vector<Vec3> positions;
    std::vector<float> distancesA;
    std::vector<float> distancesB;

    for (std::vector<IObject*>::iterator it = objects.begin(); it != objects.end(); ++it)
    {
        IResponsePhysics* response =
            static_cast<IResponsePhysics*>(L::responsePhysics(*it));
        while (response)
        {
            IObject* const responseObject = response->getObject();
            if (responseObject->hasCollisionLod(2))
            {
                ICollisionMesh* const mesh =
                    static_cast<ICollisionMesh*>(L::collisionMesh(responseObject));
                const unsigned int cid = response->getClassID();
                if (response != this
                    && cid != CID_PointResponsePhysics
                    && cid != CID_RotationalPointResponsePhysics)
                {
                    const Mat4& objectToWorld = responseObject->getAbsoluteTransformation();
                    const Mat4& worldToObject = responseObject->getInvAbsoluteTransformation();

                    int sampleCount = 0;
                    float sampleStep = 0.0f;
                    getSoldierHeight(&sampleCount, &sampleStep);            // height discarded
                    soldier->getPose();                                    // shipped discarded call
                    const float baseOffset = radius - *g_pivotHeight;

                    for (int i = 0; i < sampleCount; ++i)
                    {
                        Vec3 sample;
                        sample.x = transformation.m[12];
                        sample.y = transformation.m[13] + i * sampleStep + baseOffset;
                        sample.z = transformation.m[14];
                        if (mesh->getDistanceToSphere(
                                2, false, objectToWorld, worldToObject,
                                sample, zeroVelocity, radius,
                                normals, positions, distancesA, distancesB, 0))
                            return false;
                    }
                }
            }
            response = response->getNextToCheck();
        }
    }
    return true;
}

// the pose-change dispatch by the other object's response class.
bool SoldierResponsePhysics::expandSoldierCollision(const Vec3& a, const Vec3& b, float f,
                                                    IObject* other, float g)
{
    if (!m_enabled)
        return true;
    IResponsePhysics* response = static_cast<IResponsePhysics*>(L::responsePhysics(other));
    if (!response || response == this)
        return true;
    const unsigned int cid = response->getClassID();
    if (cid == CID_SoldierResponsePhysics)
        return soldierPoseChangeCheckVsSoldier(a, b, f, other, g);
    if (cid == CID_ResponsePhysics)
        return soldierPoseChangeCheckVsMesh(a, b, f, other, g);
    if (cid == CID_StaticResponsePhysics)
        return soldierPoseChangeCheckVsMesh(a, b, f, other, g);
    return true;
}

// soldiers never block each other's pose changes.
bool SoldierResponsePhysics::soldierPoseChangeCheckVsSoldier(const Vec3&, const Vec3&, float, IObject*, float)
{
    return true;
}

// soldierPoseChangeCheckVsMesh - / PE 0x6A3A50. The trailing float is
// unused. Retail keeps three otherwise-redundant object/transformation calls and constructs all
// four result vectors before checking collision LOD 2; their ordering is preserved.
bool SoldierResponsePhysics::soldierPoseChangeCheckVsMesh(const Vec3& start, const Vec3& end,
                                                           float radius, IObject* other,
                                                           float /*unused*/)
{
    if (!m_enabled)
        return true;

    getObject();                                                        // shipped discarded call
    const Mat4& objectToWorld = other->getAbsoluteTransformation();
    const Mat4& worldToObject = other->getInvAbsoluteTransformation();
    getObject()->getAbsoluteTransformation();                           // shipped discarded call

    std::vector<Vec3> normals;
    std::vector<Vec3> positions;
    std::vector<float> distancesA;
    std::vector<float> distancesB;

    if (other->hasCollisionLod(2))
    {
        ICollisionMesh* const mesh =
            static_cast<ICollisionMesh*>(L::collisionMesh(other));
        Vec3 velocity;
        velocity.x = end.x - start.x;
        velocity.y = end.y - start.y;
        velocity.z = end.z - start.z;
        if (mesh->getDistanceToSphere(
                2, false, objectToWorld, worldToObject, start, velocity,
                radius * 0.94999999f,
                normals, positions, distancesA, distancesB, 0))
            return false;
    }
    return true;
}

// the overrun. The impact speed is scaled by the cvar before it reaches the
// soldier, which is what makes a vehicle brush feel harder than its raw closing speed.
void SoldierResponsePhysics::hitByMovingObject(const Vec3& speed)
{
    Vec3 scaled;
    scaled.x = speed.x * *g_overrunImpactMod;
    scaled.y = speed.y * *g_overrunImpactMod;
    scaled.z = speed.z * *g_overrunImpactMod;
    static_cast<Soldier*>(m_object)->setHitByVehicleSpeed(scaled);
}

// Returns pose collision height and probe count/spacing: standing 5, crouched 3, prone 1.
// Pose 3 shares crouch. Unknown poses warn and return standing height without touching outputs.
float SoldierResponsePhysics::getSoldierHeight(int* countOut, float* stepOut)
{
    Soldier* const soldier = static_cast<Soldier*>(m_object);
    float height = soldier->getStandingHeight();                          // +0x2C0
    const SoldierPose pose = soldier->getPose();                          // +0x278
    const float radius = soldier->getSoldierRadius();                     // +0x2BC

    if (pose == 1)
    {
        height = *g_crouchHeight;
        if (countOut)
            *countOut = 3;
    }
    else if (pose < 1)
    {
        if (pose != 0)
        {
            Debug(DEBUGTYPE_6, "Physics/SoldierResponse.cpp", 597) << "Pose does not exist";
            return height;
        }
        if (countOut)
            *countOut = 5;
    }
    else if (pose == 2)
    {
        height = *g_proneHeight;
        if (countOut)
            *countOut = 1;
        if (stepOut && countOut)
            *stepOut = 0.0f;
        return height;
    }
    else if (pose == 3)
    {
        height = *g_crouchHeight;
        if (countOut)
            *countOut = 3;
    }
    else
    {
        Debug(DEBUGTYPE_6, "Physics/SoldierResponse.cpp", 597) << "Pose does not exist";
        return height;
    }

    if (stepOut && countOut)
        *stepOut = (height - (radius + radius)) / (*countOut - 1);
    return height;
}

// checkSoldierVsSoldier - / PE 0x6A1CE0. static in retail. Only unparented,
// enabled soldiers overlap; vertical spans use each response's current pose height, while the
// horizontal diameter uses the first soldier's radius twice. Both receive equal and opposite
// material-47 positional impulses. dt is unused.
void SoldierResponsePhysics::checkSoldierVsSoldier(IObject* firstObject, IObject* secondObject,
                                                    float /*dt*/, float scale)
{
    IPhysicsNode* const firstNode =
        static_cast<IPhysicsNode*>(L::physicsNode(firstObject));
    IPhysicsNode* const secondNode =
        static_cast<IPhysicsNode*>(L::physicsNode(secondObject));
    if (!firstNode->getEnabled() || !secondNode->getEnabled())
        return;
    if (L::parent(firstObject) || L::parent(secondObject))
        return;

    ++collisionStatsData->unknown18;

    SoldierResponsePhysics* const firstResponse =
        static_cast<SoldierResponsePhysics*>(L::responsePhysics(firstObject));
    SoldierResponsePhysics* const secondResponse =
        static_cast<SoldierResponsePhysics*>(L::responsePhysics(secondObject));
    const Mat4& firstTransformation = firstObject->getAbsoluteTransformation();
    const Mat4& secondTransformation = secondObject->getAbsoluteTransformation();
    const float pivotHeight = *g_pivotHeight;

    const float firstMin = firstTransformation.m[13] - pivotHeight;
    const float firstMax = firstTransformation.m[13]
                         + firstResponse->getSoldierHeight(0, 0) - pivotHeight;
    const float secondMin = secondTransformation.m[13] - pivotHeight;
    const float secondMax = secondTransformation.m[13]
                          + secondResponse->getSoldierHeight(0, 0) - pivotHeight;
    if (firstMin > secondMax || secondMin > firstMax)
        return;

    Vec3 separation;
    separation.x = secondTransformation.m[12] - firstTransformation.m[12];
    separation.y = 0.0f;
    separation.z = secondTransformation.m[14] - firstTransformation.m[14];
    const float diameter = static_cast<Soldier*>(firstObject)->getSoldierRadius()
                         * scale * 2.0f;
    const float distanceSq = separation.x * separation.x + separation.z * separation.z;
    if (distanceSq > diameter * diameter)
        return;

    const float distance = sqrtf(distanceSq);
    Vec3 normal;
    if (distance >= 0.0099999998f)
    {
        normal.x = separation.x / distance;
        normal.y = 0.0f;
        normal.z = separation.z / distance;
    }
    else
    {
        normal.x = 0.0f;
        normal.y = 0.0f;
        normal.z = 1.0f;
    }

    const float depth = diameter - distance;
    firstResponse->internalImpulseOn(Vec3::getZero(), normal, depth, 47, 47, false);
    normal.x = -normal.x;
    normal.y = -normal.y;
    normal.z = -normal.z;
    secondResponse->internalImpulseOn(Vec3::getZero(), normal, depth, 47, 47, false);

    // Shipped vestige: both ports materialise depth * normal after the calls and never read it.
}

// Sweeps a vertical stack of spheres and resolves negative primary distances.
// All results use the first material entry. dt is unused; useReturnedNormals selects the normal/distance source.
// Reduced scale skips callbacks and moving-surface bookkeeping while retaining positional impulses.
float SoldierResponsePhysics::checkSoldierVsMesh(IObject* soldierObject, IObject* otherObject,
                                                  float /*dt*/, float scale,
                                                  bool useReturnedNormals)
{
    if (!m_enabled)
        return 0.0f;
    if (L::parent(soldierObject))
        return 0.0f;

    SoldierResponsePhysics* const soldierResponse =
        static_cast<SoldierResponsePhysics*>(L::responsePhysics(soldierObject));
    IResponsePhysics* const otherResponse =
        static_cast<IResponsePhysics*>(L::responsePhysics(otherObject));

    if (!soldierObject->hasCollisionLod(1))
        return 0.0f;
    ICollisionMesh* const soldierMesh =
        static_cast<ICollisionMesh*>(L::collisionMesh(soldierObject));
    if (!otherObject->hasCollisionLod(2))
        return 0.0f;
    ICollisionMesh* const otherMesh =
        static_cast<ICollisionMesh*>(L::collisionMesh(otherObject));

    ++collisionStatsData->unknown14;

    SoldierPhysicsNode* const soldierNode =
        static_cast<SoldierPhysicsNode*>(L::physicsNode(soldierObject));
    IPhysicsNode* const otherRootNode =
        static_cast<IPhysicsNode*>(L::physicsNode(getRootParent(otherObject)));
    IPhysicsNode* const otherNode =
        static_cast<IPhysicsNode*>(L::physicsNode(otherObject));

    const Mat4& otherToWorld = otherObject->getAbsoluteTransformation();
    const Mat4& worldToOther = otherObject->getInvAbsoluteTransformation();
    soldierObject->getAbsoluteTransformation();                              // shipped discarded call

    const Mat4& soldierPrevious = soldierNode->getPrevTransformation();
    Vec3 previousPosition;
    previousPosition.x = soldierPrevious.m[12];
    previousPosition.y = soldierPrevious.m[13];
    previousPosition.z = soldierPrevious.m[14];

    Vec3 surfaceMotion;
    zero(surfaceMotion);
    if (otherResponse->getClassID() == CID_ResponsePhysics)
    {
        Mat4 inversePreviousOther = Mat4::getIdentity();
        inversePreviousOther.setInverse(otherNode->getPrevTransformation());
        const Vec3 previousInOther = transformPoint(inversePreviousOther, previousPosition);
        const Vec3 mappedPrevious =
            transformPoint(otherNode->getAbsoluteTransformation(), previousInOther);
        surfaceMotion.x = mappedPrevious.x - previousPosition.x;
        surfaceMotion.y = mappedPrevious.y - previousPosition.y;
        surfaceMotion.z = mappedPrevious.z - previousPosition.z;
    }

    const Vec3& currentForTravel = soldierObject->getAbsolutePosition();
    Vec3 relativeTravel;
    relativeTravel.x = currentForTravel.x - previousPosition.x - surfaceMotion.x;
    relativeTravel.y = currentForTravel.y - previousPosition.y - surfaceMotion.y;
    relativeTravel.z = currentForTravel.z - previousPosition.z - surfaceMotion.z;

    const Vec3& currentPosition = soldierObject->getAbsolutePosition();       // repeated in retail
    Vec3 sampleBase;
    sampleBase.x = currentPosition.x - relativeTravel.x;
    sampleBase.y = currentPosition.y - relativeTravel.y;
    sampleBase.z = currentPosition.z - relativeTravel.z;

    int sampleCount = 0;
    float sampleStep = 0.0f;
    getSoldierHeight(&sampleCount, &sampleStep);                              // return discarded
    static_cast<Soldier*>(soldierObject)->getPose();                          // shipped discarded call
    const float radius = static_cast<Soldier*>(m_object)->getSoldierRadius();
    const float baseVerticalOffset = radius - *g_pivotHeight;

    const Vec3 travelDirection = normalizedLikeEngine(relativeTravel);
    const float extensionScale = radius * 0.5f * scale * *g_extendRay;
    Vec3 halfExtension;
    halfExtension.x = travelDirection.x * extensionScale;
    halfExtension.y = travelDirection.y * extensionScale;
    halfExtension.z = travelDirection.z * extensionScale;

    std::vector<Vec3> normals;
    std::vector<Vec3> positions;
    std::vector<float> primaryDistances;
    std::vector<float> secondaryDistances;
    std::vector<int> materials;

    for (int sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex)
    {
        Vec3 sample;
        sample.x = sampleBase.x;
        sample.y = sampleBase.y + static_cast<float>(sampleIndex) * sampleStep
                 + baseVerticalOffset;
        sample.z = sampleBase.z;

        if (*g_drawSoldier == 1)
        {
            Mat4 debugSphere = Mat4::getZero();
            debugSphere.m[0] = radius;
            debugSphere.m[5] = radius;
            debugSphere.m[10] = radius;
            debugSphere.m[15] = 1.0f;
            debugSphere.m[12] = sample.x;
            debugSphere.m[13] = sample.y;
            debugSphere.m[14] = sample.z;
            drawSoldierProbe(debugSphere);
        }

        Vec3 sweepStart;
        sweepStart.x = sample.x - halfExtension.x;
        sweepStart.y = sample.y - halfExtension.y;
        sweepStart.z = sample.z - halfExtension.z;
        Vec3 sweep;
        sweep.x = relativeTravel.x + halfExtension.x;
        sweep.y = relativeTravel.y + halfExtension.y;
        sweep.z = relativeTravel.z + halfExtension.z;

        otherMesh->getDistanceToSphere(2, false, otherToWorld, worldToOther,
                                       sweepStart, sweep, radius * scale,
                                       normals, positions, primaryDistances,
                                       secondaryDistances, &materials);        // return discarded
    }

    float deepest = 0.0f;
    for (unsigned int contactIndex = 0;
         contactIndex < static_cast<unsigned int>(primaryDistances.size());
         ++contactIndex)
    {
        const float primaryDepth = primaryDistances[contactIndex];
        if (!(primaryDepth < 0.0f))
            continue;
        if (primaryDepth < deepest)
            deepest = primaryDepth;

        const Vec3& contactPosition = positions[contactIndex];
        Vec3 normal;
        float depth;
        if (useReturnedNormals)
        {
            normal = normals[contactIndex];
            depth = primaryDepth;
        }
        else
        {
            normal.x = -travelDirection.x;
            normal.y = -travelDirection.y;
            normal.z = -travelDirection.z;
            depth = secondaryDistances[contactIndex];
        }
        if (scale < 1.0f)
            depth = *g_collisionAdjustMod / scale * depth;

        Vec3 otherTangent;
        zero(otherTangent);
        Vec3 relativeSpeed;
        zero(relativeSpeed);
        int ownMaterial = 0;
        const int otherMaterial = materials[0];                              // shipped: always [0]
        bool feetContact = false;

        if (scale >= 1.0f)
        {
            ownMaterial = soldierMesh->getMaterialForVertex(1, 0) & 0xFFFF;
            otherTangent = otherRootNode->getTangentSpeed(contactPosition);
            const Vec3& soldierTangent = soldierNode->getTangentSpeed(contactPosition);
            relativeSpeed.x = soldierTangent.x - otherTangent.x;
            relativeSpeed.y = soldierTangent.y - otherTangent.y;
            relativeSpeed.z = soldierTangent.z - otherTangent.z;
            const float relativeSpeedSq = relativeSpeed.z * relativeSpeed.z
                                        + relativeSpeed.y * relativeSpeed.y
                                        + relativeSpeed.x * relativeSpeed.x;

            Vec3 relativeToSoldier;
            relativeToSoldier.x = contactPosition.x - currentPosition.x;
            relativeToSoldier.y = contactPosition.y - currentPosition.y;
            relativeToSoldier.z = contactPosition.z - currentPosition.z;

            if ((relativeSpeedSq > 0.0f || otherMaterial == 666) && *g_takeFallDamage)
            {
                if (!soldierObject->handleCollision(otherObject, relativeSpeed, normal,
                                                    relativeToSoldier, ownMaterial,
                                                    otherMaterial, 0, 0))
                    return deepest;

                if (relativeSpeedSq > 1.0f)
                    hitByMovingObject(otherTangent);

                const Vec3& otherPosition = otherObject->getAbsolutePosition();
                Vec3 relativeToOther;
                relativeToOther.x = contactPosition.x - otherPosition.x;
                relativeToOther.y = contactPosition.y - otherPosition.y;
                relativeToOther.z = contactPosition.z - otherPosition.z;
                Vec3 oppositeSpeed;
                oppositeSpeed.x = -relativeSpeed.x;
                oppositeSpeed.y = -relativeSpeed.y;
                oppositeSpeed.z = -relativeSpeed.z;
                if (otherObject->handleCollision(soldierObject, oppositeSpeed, normal,
                                                 relativeToOther, otherMaterial,
                                                 ownMaterial, 0, 0) != true)
                    return deepest;
            }

            if (sampleBase.y + baseVerticalOffset + *g_feetLevel > contactPosition.y)
            {
                feetContact = true;
                soldierNode->setCollidingMeshTangentSpeed(otherTangent);
            }
        }

        if (otherObject->getTemplate()->getClassID() == CID_GenericProjectileTemplate)
        {
            const Vec3& otherPosition = otherObject->getAbsolutePosition();
            Vec3 relativeToOther;
            relativeToOther.x = contactPosition.x - otherPosition.x;
            relativeToOther.y = contactPosition.y - otherPosition.y;
            relativeToOther.z = contactPosition.z - otherPosition.z;
            Vec3 oppositeSpeed;
            oppositeSpeed.x = -relativeSpeed.x;
            oppositeSpeed.y = -relativeSpeed.y;
            oppositeSpeed.z = -relativeSpeed.z;
            otherResponse->impulseOn(relativeToOther, oppositeSpeed, normal, -depth,
                                     ownMaterial, otherMaterial);
        }
        else
        {
            soldierResponse->internalImpulseOn(relativeSpeed, normal, depth,
                                               ownMaterial, otherMaterial, feetContact);
            if (feetContact && normal.y > m_feetContactNormal.y)
            {
                m_feetContactNormal = normal;
                m_byteFA = true;
            }
        }
    }

    return deepest;
}

// Tests projectile sweep against the coarse fly-by sphere, then the latency-compensated
// soldier skeleton for contact details. Ignores same-root shots; requires soldier armor.
void SoldierResponsePhysics::checkSoldierVsPoint(IObject* soldierObject,
                                                  IObject* projectileObject, float dt)
{
    IPhysicsNode* const soldierNode =
        static_cast<IPhysicsNode*>(L::physicsNode(soldierObject));
    IPhysicsNode* const projectileNode =
        static_cast<IPhysicsNode*>(L::physicsNode(projectileObject));

    IPlayer* owner = 0;
    if (projectileObject)
    {
        void* const projectile = projectileObject->queryInterface(IID_IProjectileObject);
        if (projectile)
            owner = projectileOwner(projectile);
        if (owner
            && getRootParent(owner->getVehicle()) == getRootParent(soldierObject))
            return;
    }

    IArmorGLComp* const armor =
        static_cast<IArmorGLComp*>(L::armorComponent(soldierObject));
    if (!armor)
        std::abort();

    const Vec3& integrationVelocity = projectileNode->getLinearIntegrationVelocity();
    Vec3 sweep;
    sweep.x = dt * integrationVelocity.x;
    sweep.y = dt * integrationVelocity.y;
    sweep.z = dt * integrationVelocity.z;

    const Vec3& projectilePosition = projectileNode->getAbsolutePosition();
    Vec3 end = projectilePosition;
    Vec3 start;
    start.x = end.x - sweep.x;
    start.y = end.y - sweep.y;
    start.z = end.z - sweep.z;
    const Vec3& soldierPosition = soldierObject->getAbsolutePosition();

    Vec3 hitPosition;
    zero(hitPosition);
    Vec3 hitNormal;
    zero(hitNormal);
    if (checkRayEntersSphere(&hitPosition, &hitNormal,
                             start, end, soldierPosition, 2.0f))
        static_cast<Soldier*>(soldierObject)->bulletFlyBy(
            projectileObject, hitPosition, hitNormal);

    float fraction = 0.0f;
    int hitPart = 0;
    int hitBone = -1;
    float latency = 0.0f;
    if (LatencyCompensation::m_isEnabled && owner)
        latency = LatencyCompensation::getLatency(owner);

    int stateIndex = 0;
    const Mat4& collisionTransformation =
        soldierNode->getLatencyCompensatedTransformation(latency, &stateIndex);
    anim::Skeleton* const skeleton = static_cast<anim::Skeleton*>(
        static_cast<Soldier*>(soldierObject)->getCollisionSkeleton(stateIndex));
    if (!skeleton)
        return;

    ++collisionStatsData->unknown10;
    if (!skeleton->checkRayVsSkeleton(
            &hitPosition, &hitNormal, &hitPart, &hitBone, &fraction,
            start, end, collisionTransformation))
        return;

    armor->setLastHitBoneIndex(hitBone, true);
    Vec3 hitDirection;
    hitDirection.x = end.x - start.x;
    hitDirection.y = end.y - start.y;
    hitDirection.z = end.z - start.z;
    armor->setLastHitDirection(hitDirection, true);

    Vec3 relativeSpeed;
#if defined(_MSC_VER)
    // PE evaluates the source expression's right operand first; ELF evaluates its left first.
    const Vec3& soldierSpeed = soldierNode->getPositionalSpeed();
    const Vec3& projectileSpeed = projectileNode->getPositionalSpeed();
#else
    const Vec3& projectileSpeed = projectileNode->getPositionalSpeed();
    const Vec3& soldierSpeed = soldierNode->getPositionalSpeed();
#endif
    relativeSpeed.x = projectileSpeed.x - soldierSpeed.x;
    relativeSpeed.y = projectileSpeed.y - soldierSpeed.y;
    relativeSpeed.z = projectileSpeed.z - soldierSpeed.z;

    PointResponsePhysics* const projectileResponse =
        static_cast<PointResponsePhysics*>(L::responsePhysics(projectileObject));
    projectileResponse->doResponse(
        projectileObject, soldierObject, hitPosition, 0.0f, hitNormal,
        hitPart, 0, relativeSpeed, soldierNode);
}

// The impulse the two "nothing left to resolve" exits share. GCC emits it twice (0x837719E for the
// early-out arm and 0x8377663 for the loop's no-contact arm); it is identical both times, so it
// lives here once. The scale is (elasticity + 1) * phy-imp-mod, applied to the accumulated speed
// adjust, at the soldier's newly committed position.
static void applyAccumulatedImpulse(IPhysicsNode* node, const Vec3& at, const Vec3& speedAdjust,
                                    float elasticity)
{
    if (!(speedAdjust.y * speedAdjust.y + speedAdjust.x * speedAdjust.x
          + speedAdjust.z * speedAdjust.z > 0.0f))
        return;

    const float bounce = elasticity + 1.0f;
    Vec3 scaled;
    scaled.x = speedAdjust.x * bounce;
    scaled.y = speedAdjust.y * bounce;
    scaled.z = speedAdjust.z * bounce;
    Vec3 impulse;
    impulse.x = scaled.x * *g_impMod;
    impulse.y = scaled.y * *g_impMod;
    impulse.z = scaled.z * *g_impMod;
    node->addRelativeImpulse(at, impulse);                                // +0x160
}

// Commits push-out and iterates collision tests only on the authoritative outermost call.
// Recursive calls commit a round's adjustment; remote ghosts never iterate. Early rounds exclude
// deepest-penetration tracking. Exhausting the limit restores the previous position and can apply crush damage.
// Queries exclude projectiles and require flags 0x1000100; retain the discarded radius getter.
void SoldierResponsePhysics::solveImpulse(IPhysicsNode* node)
{
    if (!m_enabled)
        return;

    ++g_solveImpulseDepth;

    const Mat4& prevTransformation = node->getPrevTransformation();       // +0x50

    Vec3 newPosition = m_positionalAdjusts1;
    Vec3 speedAdjust = m_positionalAdjusts2;
    const float elasticity = m_averageElasticity;

    const Vec3& position = node->getAbsolutePosition();                   // +0x48
    newPosition.x += position.x;
    newPosition.y += position.y;
    newPosition.z += position.z;
    node->setAbsolutePosition(newPosition);                               // +0x4C
    zero(m_positionalAdjusts1);
    zero(m_positionalAdjusts2);

    IObject* const object = getObject();                                  // +0x40
    io::INetworkable* const networkable = object->getNetworkable();       // slot 54
    // ghost-active but not in the network = a replicated copy of somebody else's object; it does
    // not resolve its own collision. A new witness for INetworkable +0x14's role.
    const bool remoteGhost =
        networkable && networkable->m_ghostActive && !networkable->m_isInNetwork;

    if (remoteGhost || *g_testCount <= 0 || g_solveImpulseDepth != 1)
    {
        if (*g_testCount == 0)
            applyAccumulatedImpulse(node, newPosition, speedAdjust, elasticity);
        --g_solveImpulseDepth;
        return;
    }

    // Saved across the iteration and put back at the end - the rounds use both as scratch.
    int restoreContactCount = m_contactCount;
    Vec3 restoreAverageSpeed = m_positionalAdjusts3;

    std::vector<IObject*>& objects = objectManager->getObjectTempVector();     // +0xA8
    ObjectIgnoreInterfacePredicator predicator(0x1000100u, IID_IProjectileObject);
    objects.resize(0);

    static_cast<Soldier*>(m_object)->getSoldierRadius();                  // +0x2BC discarded
    const float radius = m_object->getPhysicBoundingRadius();             // slot 38
    objectManager->getObjectsFromGrid(node->getAbsolutePosition(), radius,
                                      objects, &predicator);              // +0x40

    float deepest = 0.0f;
    const unsigned int groups = getCollisionGroups();                     // +0x54
    int round = 0;
    bool hitByMoving = false;
    IObject* crusher = 0;
    Vec3 crusherSpeed;
    zero(crusherSpeed);

    while (round < *g_testCount)
    {
        m_contactCount = 0;
        zero(m_positionalAdjusts3);
        const bool earlyRound = round < *g_testCountStartBackAt;

        for (std::vector<IObject*>::iterator it = objects.begin(); it != objects.end(); ++it)
        {
            IObject* const other = *it;
            if (other->getTemplate()->getClassID() == CID_ZiplineTemplate)      // +0x14, then +0x10
                continue;
            if (other->getTemplate()->getClassID() == CID_ZiplineRopeTemplate)
                continue;
            IResponsePhysics* response =
                static_cast<IResponsePhysics*>(L::responsePhysics(other));      // +0x48
            if (!response)
                continue;
            IPhysicsNode* const otherNode =
                static_cast<IPhysicsNode*>(L::physicsNode(other));              // +0x44
            do
            {
                IObject* const responseObject = response->getObject();          // +0x40
                if (responseObject->hasCollisionLod(2))                         // slot 51
                {
                    const unsigned int cid = response->getClassID();            // +0x24
                    if (response != this
                        && cid != CID_PointResponsePhysics
                        && cid != CID_RotationalPointResponsePhysics
                        && (response->getCollisionGroups() & 0x10) == 0)        // +0x54
                    {
                        // heavy, awake and moving -> a crush candidate for the rounds-exhausted arm
                        if (otherNode->getMass() > 100.0f                       // +0xDC
                            && !otherNode->getIsSleeping())                     // +0x11C
                        {
                            crusherSpeed = otherNode->getPositionalSpeed();     // +0x60
                            const Vec3& myPosition = getObject()->getAbsolutePosition();   // slot 27
                            const Vec3& tangent = otherNode->getTangentSpeed(myPosition);  // +0xA4
                            const Vec3& mySpeed = node->getPositionalSpeed();   // +0x60
                            Vec3 closing;
                            closing.x = tangent.x - mySpeed.x;
                            closing.y = tangent.y - mySpeed.y;
                            closing.z = tangent.z - mySpeed.z;
                            hitByMoving = true;
                            if (closing.z * closing.z + closing.y * closing.y
                                    + closing.x * closing.x > *g_crushMinPosSpeedSq)
                                crusher = otherNode->getObject();               // +0x2C
                        }

                        if (cid == CID_SoldierResponsePhysics)
                        {
                            // static: the ELF passes the soldier IObject* in the `this` slot.
                            checkSoldierVsSoldier(getObject(), responseObject,
                                                  k30HzTick, *g_testRadiusMod);
                        }
                        else if ((response->getCollisionGroups() & groups) == 0)
                        {
                            const float d = checkSoldierVsMesh(object, responseObject, k30HzTick,
                                                               *g_testRadiusMod, earlyRound);
                            if (deepest > d)
                                deepest = d;
                        }
                    }
                }
                response = response->getNextToCheck();                          // +0x60
            } while (response);
        }

        const float terrain = internal_checkVsTerrain(k30HzTick, *g_testRadiusMod);
        if (!earlyRound && deepest > terrain)
            deepest = terrain;

        if (!m_contactCount)
            break;
        solveImpulse(node);                                               // +0x34, the depth-2 commit
        ++round;
    }

    bool damaged = false;
    if (round >= *g_testCount)
    {
        // Ran out of rounds: give up on resolving and put the soldier back
        node->reset(false);                                               // +0x118
        m_byteFA = true;
        m_feetContactNormal.x = 0.0f;
        m_feetContactNormal.y = 1.0f;
        m_feetContactNormal.z = 0.0f;
        restoreContactCount = 1;
        zero(restoreAverageSpeed);
        node->setAbsolutePosition(
            *reinterpret_cast<const Vec3*>(&prevTransformation.m[12]));   // +0x4C

        if (hitByMoving && crusher && -*g_crushDepth >= deepest)
        {
            hitByMovingObject(crusherSpeed);
            if (gameLogic)
            {
                Vec3 up;
                up.x = 0.0f;
                up.y = 1.0f;
                up.z = 0.0f;
                Vec3 at = m_object->getAbsolutePosition();
                gameLogic->giveDamage(m_object, 1000.0f, 0, 0, crusher, -1, 0, 0, at, up, 78,
                                      false, true, true);                 // IGameLogic +0xE4
            }
            damaged = true;
        }
    }
    else
    {
        applyAccumulatedImpulse(node, newPosition, speedAdjust, elasticity);
    }

    if (!damaged)
        checkIfCrushedByVehicle();

    m_contactCount = restoreContactCount;
    m_positionalAdjusts3 = restoreAverageSpeed;
    --g_solveImpulseDepth;
}

// Checks stacked half-radius spheres against vehicle LOD 2 for crushing.
// Both vehicle speed and probe closing speed must exceed thresholds. A hit deals 1000 damage
// with material 78 attributed to the vehicle. Copy scratch results before requerying the grid.
void SoldierResponsePhysics::checkIfCrushedByVehicle()
{
    std::vector<IObject*> objects(objectManager->getObjectTempVector());   // +0xA8, a copy
    ObjectIgnoreInterfacePredicator predicator(0x1000108u, IID_IProjectileObject);

    const Mat4& transformation = getObject()->getAbsoluteTransformation();  // +0x40, then slot 33
    Soldier* const soldier = static_cast<Soldier*>(m_object);
    const float soldierRadius = soldier->getSoldierRadius();               // +0x2BC
    const float radius = m_object->getPhysicBoundingRadius();              // slot 38
    objectManager->getObjectsFromGrid(*reinterpret_cast<const Vec3*>(&transformation.m[12]),
                                      radius, objects, &predicator);       // +0x40

    const Vec3& probeVelocity = Vec3::getZero();
    IPhysicsNode* const node = static_cast<IPhysicsNode*>(L::physicsNode(m_object));

    for (std::vector<IObject*>::iterator it = objects.begin(); it != objects.end(); ++it)
    {
        IObject* const other = *it;
        IResponsePhysics* response =
            static_cast<IResponsePhysics*>(L::responsePhysics(other));      // +0x48
        if (!response)
            continue;
        IPhysicsNode* const otherNode =
            static_cast<IPhysicsNode*>(L::physicsNode(other));              // +0x44
        do
        {
            IObject* const responseObject = response->getObject();          // +0x40
            const unsigned int cid = response->getClassID();                // +0x24
            if (responseObject->hasCollisionLod(2)                          // slot 51
                && response != this
                && cid != CID_PointResponsePhysics
                && cid != CID_RotationalPointResponsePhysics
                && (response->getCollisionGroups() & 0x10) == 0             // +0x54
                && otherNode->getMass() >= 100.0f                           // +0xDC
                && !otherNode->getIsSleeping())                             // +0x11C
            {
                const Mat4& objectToWorld = responseObject->getAbsoluteTransformation();     // slot 33
                const Mat4& worldToObject = responseObject->getInvAbsoluteTransformation();  // slot 34
                float step = 0.0f;
                int count = 0;
                getSoldierHeight(&count, &step);                            // return discarded
                soldier->getPose();                                         // +0x278 discarded
                const float base = soldierRadius - *g_pivotHeight;

                for (int i = 0; i < count; ++i)
                {
                    Vec3 probe;
                    probe.x = transformation.m[12];
                    probe.y = transformation.m[13] + static_cast<float>(i) * step + base;
                    probe.z = transformation.m[14];

                    std::vector<Vec3> outNormals;
                    std::vector<Vec3> outPositions;
                    std::vector<float> outDistancesA;
                    std::vector<float> outDistancesB;
                    // ELF 0x8377E33 / PE 0x6A2A92: use this response's mesh with its transforms.
                    // The grid candidate can be the root of a chain of offset vehicle parts.
                    ICollisionMesh* const mesh =
                        static_cast<ICollisionMesh*>(L::collisionMesh(responseObject)); // +0x40
                    if (!mesh->getDistanceToSphere(2, false, objectToWorld, worldToObject, probe,
                                                   probeVelocity, soldierRadius * 0.5f,
                                                   outNormals, outPositions,
                                                   outDistancesA, outDistancesB, 0))   // slot 25
                        continue;

                    const Vec3& tangent = otherNode->getTangentSpeed(probe);          // +0xA4
                    const Vec3& mySpeed = node->getPositionalSpeed();                 // +0x60
                    Vec3 closing;
                    closing.x = tangent.x - mySpeed.x;
                    closing.y = tangent.y - mySpeed.y;
                    closing.z = tangent.z - mySpeed.z;

                    const Vec3& vehicleSpeed = otherNode->getPositionalSpeed();       // +0x60
                    if (!(vehicleSpeed.x * vehicleSpeed.x + vehicleSpeed.y * vehicleSpeed.y
                              + vehicleSpeed.z * vehicleSpeed.z > *g_crushMinVehicleSpeedSq))
                        continue;
                    if (!(closing.x * closing.x + closing.y * closing.y
                              + closing.z * closing.z > *g_crushMinPosSpeedSq))
                        continue;

                    IObject* const crusher = otherNode->getObject();                  // +0x2C
                    if (gameLogic)
                    {
                        Vec3 up;
                        up.x = 0.0f;
                        up.y = 1.0f;
                        up.z = 0.0f;
                        Vec3 at = m_object->getAbsolutePosition();                    // slot 27
                        gameLogic->giveDamage(m_object, 1000.0f, 0, 0, crusher, -1, 0, 0, at, up,
                                              78, false, true, true);                 // +0xE4
                    }
                    return;
                }
            }
            response = response->getNextToCheck();                          // +0x60
        } while (response);
    }
}

// Applies resistance and static/kinetic friction after collision resolution.
// Flag bit 6 tracks sticking. Exceeding the static cap starts sliding; meeting the kinetic cap
// resticks without friction that tick. Contact/grip or floating is required; resistance is uncapped.
void SoldierResponsePhysics::addFriction(IPhysicsNode* node)
{
    if (!m_enabled)
        return;

    if (!((m_byteFA && getPermanentGrip()) || m_isFloating))               // +0x80
    {
        m_frictionFlags = 1;
        return;
    }

    const float friction = m_averageFriction;
    const float n = m_feetContactNormal.y;
    const float n5 = n * (n * n * (n * n));
    const float staticCap  = friction * 7.1999998f * 9.8199997f * n5 / 30.0f;
    const float kineticCap = friction * 4.8000002f * 9.8199997f * n5 / 30.0f;

    Vec3 relative;
    relative.x = m_positionalAdjusts3.x - m_surfacePositionalSpeed.x;
    relative.y = m_positionalAdjusts3.y - m_surfacePositionalSpeed.y;
    relative.z = m_positionalAdjusts3.z - m_surfacePositionalSpeed.z;
    relative.y += static_cast<IPhysicsSystem*>(basicPhysicsSystem)->getGravity() / 30.0f;   // +0x20

    // remove the component along the feet normal
    Vec3 alongNormal;
    const float nn = m_feetContactNormal.y * m_feetContactNormal.y
                   + m_feetContactNormal.x * m_feetContactNormal.x
                   + m_feetContactNormal.z * m_feetContactNormal.z;
    if (fabsf(nn) > FLT_EPSILON)
    {
        const float k = (relative.y * m_feetContactNormal.y
                         + m_feetContactNormal.x * relative.x
                         + relative.z * m_feetContactNormal.z) / nn;
        alongNormal.x = m_feetContactNormal.x * k;
        alongNormal.y = m_feetContactNormal.y * k;
        alongNormal.z = m_feetContactNormal.z * k;
    }
    else
    {
        alongNormal.x = alongNormal.y = alongNormal.z = 0.0f;
    }

    Vec3 tangential;
    tangential.x = relative.x - alongNormal.x;
    tangential.y = relative.y - alongNormal.y;
    tangential.z = relative.z - alongNormal.z;

    if (m_averageResistance > 0.0f)
    {
        Vec3 resistance;
        resistance.x = -tangential.x * m_averageResistance;
        resistance.y = -tangential.y * m_averageResistance;
        resistance.z = m_averageResistance * -tangential.z;
        node->addAccelerationAtRelativePosition(Vec3::getZero(), resistance);   // +0x94
    }

    Vec3 demand;
    demand.x = -tangential.x;
    demand.y = -tangential.y;
    demand.z = -tangential.z;

    if ((m_frictionFlags & 0x40) != 0)
    {
        // sticking: only a demand past the static cap breaks it
        const float sq = demand.z * demand.z + demand.y * demand.y + demand.x * demand.x;
        bool broke = false;
        if (sq > staticCap * staticCap && sq > 0.0000010000001f)
        {
            broke = true;
            const float s = fabsf(staticCap) / sqrtf(sq);
            demand.x = demand.x * s;
            demand.y = demand.y * s;
            demand.z = s * demand.z;
        }
        if (broke)
        {
            m_frictionFlags = static_cast<unsigned char>(m_frictionFlags & ~0x40u);
            const float sq2 = demand.z * demand.z + demand.y * demand.y + demand.x * demand.x;
            if (sq2 > kineticCap * kineticCap && sq2 > 0.0000010000001f)
            {
                const float s = fabsf(kineticCap) / sqrtf(sq2);
                demand.x = demand.x * s;
                demand.y = demand.y * s;
                demand.z = s * demand.z;
            }
        }
    }
    else
    {
        // sliding: a demand within the kinetic cap re-establishes the stick
        const float sq = demand.z * demand.z + demand.y * demand.y + demand.x * demand.x;
        if (sq <= kineticCap * kineticCap || sq <= 0.0000010000001f)
        {
            m_frictionFlags = static_cast<unsigned char>(m_frictionFlags | 0x40u);
        }
        else
        {
            const float s = fabsf(kineticCap) / sqrtf(sq);
            demand.x = demand.x * s;
            demand.y = demand.y * s;
            demand.z = s * demand.z;
        }
    }

    demand.x = demand.x * 30.0f;
    demand.y = demand.y * 30.0f;
    demand.z = 30.0f * demand.z;

    Vec3 at = node->getAbsolutePosition();                                // +0x48
    node->addFrictionAtAbsolutePosition(at, demand);                      // +0x98

    zero(m_surfacePositionalSpeed);
    m_contactCount = 0;
    zero(m_positionalAdjusts3);
}

} // namespace world
} // namespace hfe
} // namespace dice
