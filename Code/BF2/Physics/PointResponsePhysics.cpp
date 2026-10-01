// Point collision responses against terrain and objects. [INFERRED] Source path.

#include "PointResponsePhysics.h"

#include <math.h>
#include <float.h>
#include <string.h>

#include "../Core/Ids.h"
#include "../Game/Objects/IObject.h"
#include "../Game/Objects/IPlayer.h"
#include "../Game/Objects/IObjectTemplate.h"          // solveImpulse: the contact object's template CID
#include "../Game/Objects/ObjectLayoutAccess.h"       // the object's +0x44 node, +0x48 response
#include "CollisionStats.h"                           // the per-frame terrain-query counter
#include "CollisionManager.h"
#include "GeomUtil.h"
#include "ICollisionMesh.h"
#include "LatencyCompensation.h"
#include "../Math/Mat4.h"
#include "../Scene/Object/ObjectManager.h"
#include "../Scene/Object/ObjectUtil.h"
#include "HeightmapCluster.h"                         // intersectRayInWorldCoords / getWaterLevelInWorldCoords
#include "IPhysicsNode.h"

namespace dice {
namespace hfe {
namespace world {

extern IBase* heightmapCluster;                       // ds:0x8ABAF24 - Core/Globals.cpp
IPlayer* projectileOwner(void* projectile);            // typed IProjectileObject seam in GenericProjectile.cpp

// Wake speed, impulse nudge and debug-distance tunables.
// [CHOSEN] Representation flattens original value/pointer pairs to their values.
namespace {
float s_wakeSpeedThreshold = 0.0099999998f;
float s_nudgeMagnitude     = 10.0f;
float s_debugLineDistance  = 0.0f;                    // 0x8ABA808; name [INFERRED]
} // namespace

// StaticResponsePhysics ctor, then the defaults.
PointResponsePhysics::PointResponsePhysics()
{
    m_contactDepthSq = -1.0f;
    m_refCount = 1;
    m_collisionChecked = 0;
    m_contactObject = 0;
    m_contactPoint.x = m_contactPoint.y = m_contactPoint.z = 0.0f;
    m_contactPosition.x = m_contactPosition.y = m_contactPosition.z = 0.0f;
    m_contactSurfaceSpeed.x = m_contactSurfaceSpeed.y = m_contactSurfaceSpeed.z = 0.0f;
    m_contactSpeed.x = m_contactSpeed.y = m_contactSpeed.z = 0.0f;
    m_contactScale = 0.0f;
    m_contactFraction = 0.0f;
    m_contactNormal.x = m_contactNormal.y = m_contactNormal.z = 0.0f;
    m_contactOwnMaterial = 0;
    m_contactMaterial = 0;
    m_hasFaceInfo = false;
    m_faceInfo = 0;
    m_underWaterDepth = 0.0f;
}

PointResponsePhysics::~PointResponsePhysics() {}

void PointResponsePhysics::addRef()  { ++m_refCount; }
int  PointResponsePhysics::getRef()  { return m_refCount; }
int  PointResponsePhysics::release()
{
    const int r = --m_refCount;
    if (r == 0) delete this;
    return r;
}

void* PointResponsePhysics::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase)             return const_cast<PointResponsePhysics*>(this);
    if (iid == IID_IResponsePhysics)  return const_cast<PointResponsePhysics*>(this);
    return 0;
}

unsigned int PointResponsePhysics::getClassID() const              { return CID_PointResponsePhysics; }
void PointResponsePhysics::setCollisionChecked(unsigned int v)     { m_collisionChecked = v; }
unsigned int PointResponsePhysics::getCollisionChecked() const     { return m_collisionChecked; }

// Sweeps the point through collision LOD 0 with an optional compensated target pose.
// Requires the own node and preserves owner/root exclusion, repeated reads, and
// inverse-before-forward transform order. The trailing bool is unused.
void PointResponsePhysics::checkObjectVsObject(IObject* object, IObject* other, float dt, bool)
{
    typedef object_detail::LayoutAccess L;
    if (!m_enabled)
        return;
    if (object)
    {
        void* const projectile = object->queryInterface(IID_IProjectileObject);
        if (projectile)
        {
            IPlayer* const owner = projectileOwner(projectile);
            if (owner)
            {
                IObject* const ownerRoot = getRootParent(owner->getVehicle());
                if (ownerRoot == getRootParent(other))
                    return;
            }
        }
    }

    IPhysicsNode* const node = static_cast<IPhysicsNode*>(L::physicsNode(object));
    if (!L::responsePhysics(other))
        return;
    if (!other->hasCollisionLod(0))
        return;
    ICollisionMesh* const mesh = static_cast<ICollisionMesh*>(L::collisionMesh(other));
    IPhysicsNode* const ownNode = static_cast<IPhysicsNode*>(L::physicsNode(object));
    IPhysicsNode* const otherRootNode = static_cast<IPhysicsNode*>(L::physicsNode(getRootParent(other)));
    (void)object->getAbsoluteTransformation();
    const Vec3& velocity = node->getLinearIntegrationVelocity();
    const Vec3 sweep = {dt * velocity.x, dt * velocity.y, dt * velocity.z};
    const Vec3 end = node->getAbsolutePosition();
    const Vec3 start = {end.x - sweep.x, end.y - sweep.y, end.z - sweep.z};
    Vec3 normal = {}, position = {};
    float planeDistance, along;
    int material = 0;
    int face = 0;                                    // CollisionFaceInfo is a four-byte POD
    CollisionFaceInfo* const faceInfo = reinterpret_cast<CollisionFaceInfo*>(&face);
    ++collisionStatsData->unknown08;

    bool hit;
    IPhysicsNode* const otherNode = static_cast<IPhysicsNode*>(L::physicsNode(other));
    if (LatencyCompensation::m_isEnabled && otherNode && otherNode->hasLatencyCompensation())
    {
        float latency = 0.0f;
        void* const projectile = object->queryInterface(IID_IProjectileObject);
        if (projectile)
            latency = LatencyCompensation::getLatency(projectileOwner(projectile));
        const Mat4& transform = otherNode->getLatencyCompensatedTransformation(latency, 0);
        Mat4 inverse = Mat4::getIdentity();
        inverse.setInverse(transform);
        hit = mesh->getDistance(0, false, transform, inverse, start, sweep, normal, position,
                                planeDistance, along, &material, faceInfo, true, false);
    }
    else
    {
        const Mat4& inverse = other->getInvAbsoluteTransformation();
        const Mat4& transform = other->getAbsoluteTransformation();
        hit = mesh->getDistance(0, false, transform, inverse, start, sweep, normal, position,
                                planeDistance, along, &material, faceInfo, true, false);
    }

    if (s_debugLineDistance > 0.0f)
    {
        Mat4 camera = Mat4::getIdentity();
        objectManager->getCameraInfo(&camera, 0, 0);
        Vec3 lineStart = start;
        const Vec3& cameraPosition = *reinterpret_cast<const Vec3*>(&camera.m[12]);
        const float distance = getClosestDistanceToLine(cameraPosition, &end, &lineStart);
        if (s_debugLineDistance > distance)
            collisionManager->drawLine(end, lineStart, 0xFF0000FFu);
    }
    if (hit)
    {
        // [PLATFORM] The two ports evaluate these virtual speed accessors in opposite orders.
#if defined(_WIN32)
        const Vec3& surfaceSpeed = otherRootNode ? otherRootNode->getTangentSpeed(position) : Vec3::getZero();
        const Vec3& ownSpeed = ownNode->getPositionalSpeed();
#else
        const Vec3& ownSpeed = ownNode->getPositionalSpeed();
        const Vec3& surfaceSpeed = otherRootNode ? otherRootNode->getTangentSpeed(position) : Vec3::getZero();
#endif
        const Vec3 relativeSpeed = {ownSpeed.x - surfaceSpeed.x, ownSpeed.y - surfaceSpeed.y,
                                   ownSpeed.z - surfaceSpeed.z};
        doResponse(object, other, position, planeDistance, normal, material, faceInfo,
                   relativeSpeed, otherRootNode);
    }
}

// Records only a deeper object contact. Requires a valid other object; the final node argument is unused.
// Preserve both position queries, the other object's position in the surface-speed field and the
// four-byte opaque face-info copy.
void PointResponsePhysics::doResponse(IObject* object, IObject* other, const Vec3& point,
                                      float fraction, const Vec3& normal, int material,
                                      CollisionFaceInfo* faceInfo, const Vec3& speed,
                                      IPhysicsNode*)
{
    const Vec3& position = object->getAbsolutePosition();
    const float dx = point.x - position.x;
    const float dy = point.y - position.y;
    const float dz = point.z - position.z;
    const float depthSq = dz * dz + dy * dy + dx * dx;
    if (!(depthSq > m_contactDepthSq))
        return;

    m_contactDepthSq = depthSq;
    m_contactObject = other;
    m_contactPoint = point;

    const Vec3& contactPosition = object->getAbsolutePosition();
    m_contactPosition = contactPosition;
    m_contactSurfaceSpeed = other->getAbsolutePosition();
    m_contactSpeed = speed;
    m_contactScale = 1.0f;
    m_contactFraction = fraction;
    m_contactNormal = normal;
    m_contactOwnMaterial = 0;
    m_contactMaterial = material;
    m_hasFaceInfo = false;
    if (faceInfo)
    {
        m_hasFaceInfo = true;
        memcpy(&m_faceInfo, faceInfo, sizeof(m_faceInfo));
    }
}

// Sweeps the point's previous-to-current position against terrain, retaining the deepest contact.
// Disabled responses and collision-group bit 0 skip the test. Material 1 denotes water: nudge the
// contact by 1 mm with the vertical motion without changing node submersion; land updates submersion.
void PointResponsePhysics::checkVsTerrain(float dt)
{
    typedef object_detail::LayoutAccess L;

    if (!m_enabled)
        return;
    if ((getCollisionGroups() & 1) != 0)                                  // +0x54
        return;

    IObject* const object = m_object;
    const Vec3& position = object->getAbsolutePosition();                 // slot 27
    IPhysicsNode* const node = static_cast<IPhysicsNode*>(L::physicsNode(object));   // +0x44
    const Vec3& velocity = node->getLinearIntegrationVelocity();          // +0x16C
    Vec3 sweep;
    sweep.x = dt * velocity.x;
    sweep.y = dt * velocity.y;
    sweep.z = dt * velocity.z;
    Vec3 from;
    from.x = position.x - sweep.x;
    from.y = position.y - sweep.y;
    from.z = position.z - sweep.z;

    Vec3 hitPoint;
    hitPoint.x = hitPoint.y = hitPoint.z = 0.0f;
    float hitFraction = 0.0f;
    Vec3 hitNormal;
    hitNormal.x = hitNormal.y = hitNormal.z = 0.0f;
    unsigned char material = 0;

    ++collisionStatsData->heightAndNormalQueries;                          // +0x0C
    if (!static_cast<IHeightmapCluster*>(heightmapCluster)->intersectRayInWorldCoords(
            from, position, hitPoint, hitFraction, hitNormal, material, true))
        return;

    const float dx = hitPoint.x - position.x;
    const float dy = hitPoint.y - position.y;
    const float dz = hitPoint.z - position.z;
    const float depthSq = dz * dz + dy * dy + dx * dx;
    if (!(depthSq > m_contactDepthSq))
        return;

    m_contactDepthSq = depthSq;
    m_contactObject = 0;
    m_contactPoint = hitPoint;
    m_contactPosition = position;
    m_contactSurfaceSpeed = Vec3::getZero();
    m_contactSpeed = node->getPositionalSpeed();                          // +0x60
    m_contactScale = 1.0f;
    m_contactFraction = hitFraction;
    m_contactNormal = hitNormal;
    m_contactOwnMaterial = 0;
    m_hasFaceInfo = false;
    m_contactMaterial = material;

    if (material == 1)                                                    // the water surface
    {
        if (0.0f < sweep.y)
        {
            m_contactPoint.y = m_contactPoint.y + 0.001f;
            m_underWaterDepth = 0.0f;
        }
        else
        {
            m_contactPoint.y = m_contactPoint.y - 0.001f;
            m_underWaterDepth = 0.001f;
        }
        return;
    }

    const float depth = m_contactPoint.y
        - static_cast<IHeightmapCluster*>(heightmapCluster)->getWaterLevelInWorldCoords(m_contactPoint);
    if (0.0f <= depth)
        m_underWaterDepth = 0.0f;
    else
        m_underWaterDepth = -depth;
    node->setUnderWater(m_underWaterDepth);                               // +0x108
}

// Delivers an enabled recorded contact, updates submersion and clears the frame record.
// Object contacts may wake/nudge light mobile roots; terrain contacts end after clearing.
// Spring/soldier templates are excluded from the nudge.
void PointResponsePhysics::solveImpulse(IPhysicsNode* node)
{
    typedef object_detail::LayoutAccess L;

    if (!m_enabled || m_contactDepthSq < 0.0f)
        return;

    Vec3 penetration;
    penetration.x = m_contactPoint.x - m_contactPosition.x;
    penetration.y = m_contactPoint.y - m_contactPosition.y;
    penetration.z = m_contactPoint.z - m_contactPosition.z;

    IObject* const object = getObject();                                  // +0x40
    CollisionFaceInfo* const faceInfo =
        m_hasFaceInfo == true ? reinterpret_cast<CollisionFaceInfo*>(&m_faceInfo) : 0;
    object->handleCollision(m_contactObject, m_contactSpeed, m_contactNormal, penetration,
                            m_contactOwnMaterial, m_contactMaterial, 0, faceInfo);   // slot 45
    node->setUnderWater(m_underWaterDepth);                               // +0x108
    m_contactDepthSq = -1.0f;

    IObject* const other = m_contactObject;
    if (!other)
        return;
    const unsigned int cid = L::objectTemplate(other)->getClassID();      // +0x14 -> +0x10
    if (cid == CID_SpringTemplate || cid == CID_SoldierTemplate)
        return;
    IPhysicsNode* const otherNode =
        static_cast<IPhysicsNode*>(L::physicsNode(getRootParent(other)));
    if (!otherNode)
        return;
    if (!otherNode->getIsMobile())                                        // +0x134
        return;
    const float mass = otherNode->getMass();                              // +0xDC
    if (!(mass < 100.0f))
        return;

    if (otherNode->getIsSleeping())                                       // +0x11C
    {
        const float into = -(m_contactNormal.x * m_contactSpeed.x
                           + m_contactNormal.y * m_contactSpeed.y
                           + m_contactNormal.z * m_contactSpeed.z);
        if (into > s_wakeSpeedThreshold)
            otherNode->setIsAwake();                                      // +0x130
    }
    if (otherNode->getIsSleeping())
        return;

    float nx = -m_contactSpeed.x;
    float ny = -m_contactSpeed.y;
    float nz = -m_contactSpeed.z;
    const float lengthSq = nz * nz + ny * ny + nx * nx;
    if (fabsf(lengthSq - 1.0f) >= FLT_EPSILON)
    {
        if (fabsf(lengthSq) >= FLT_EPSILON)
        {
            const float inv = 1.0f / sqrtf(lengthSq);
            nx = nx * inv;
            ny = ny * inv;
            nz = nz * inv;
        }
        else
        {
            nx = 0.0f;
            ny = 0.0f;
            nz = 0.0f;
        }
    }
    Vec3 direction;
    direction.x = -nx;
    direction.y = -ny;
    direction.z = -nz;
    const float invMass = 1.0f / mass;
    Vec3 acceleration;
    acceleration.x = nx * s_nudgeMagnitude * invMass;
    acceleration.y = ny * s_nudgeMagnitude * invMass;
    acceleration.z = invMass * (nz * s_nudgeMagnitude);

    const Vec3& otherPosition = other->getAbsolutePosition();             // slot 27
    Vec3 relative;
    relative.x = m_contactPoint.x - otherPosition.x;
    relative.y = m_contactPoint.y - otherPosition.y;
    relative.z = m_contactPoint.z - otherPosition.z;
    static_cast<IResponsePhysics*>(L::responsePhysics(other))
        ->impulseOn(relative, acceleration, direction, 0.0f, 0, 0);      // +0x30
}

} // namespace world
} // namespace hfe
} // namespace dice
