// Rotational-point collision responses. [INFERRED] Source path.

#include "RotationalPointResponsePhysics.h"

#include <float.h>
#include <math.h>

#include "../Core/Ids.h"
#include "../Game/Objects/IObject.h"
#include "../Game/Objects/IObjectTemplate.h"
#include "../Game/Objects/ObjectLayoutAccess.h"       // the object's +0x44 node
#include "BasicPhysicsSystem.h"                       // basicPhysicsSystem / getGravity
#include "CollisionStats.h"                           // the per-frame counters at +0x20 / +0x24
#include "HeightmapCluster.h"                         // getHeightAndNormalInWorldCoords / water level
#include "ICollisionMesh.h"                           // the other object's LOD-0 ray query
#include "IPhysicsNode.h"
#include "Material.h"                                 // getHasWaterPhysics
#include "MaterialManager.h"                          // getMaterialPtr / friction / elasticity

namespace dice {
namespace hfe {
namespace world {

extern IBase* heightmapCluster;                       // ds:0x8ABAF24 - Core/Globals.cpp
extern IBase* materialManager;                        // ds:0x8AD2820 - Core/Globals.cpp

// [CHOSEN] Names for mutable object-collision settings, retaining original values and initialization order.
namespace {
int   s_objectCollisionIterations       = 3;
float s_objectCollisionMinDirectionRatio = 0.05f;
float s_objectCollisionSurfaceOffset     = 0.001f;

inline void normalizeObjectCollisionDirection(Vec3& value)
{
    const float lengthSq = value.y * value.y + value.x * value.x + value.z * value.z;
    if (fabsf(lengthSq - 1.0f) >= FLT_EPSILON)
    {
        if (fabsf(lengthSq) >= FLT_EPSILON)
        {
            const float invLength = 1.0f / sqrtf(lengthSq);
            value.x = value.x * invLength;
            value.y = value.y * invLength;
            value.z = invLength * value.z;
        }
        else
        {
            value.x = value.y = value.z = 0.0f;
        }
    }
}
} // namespace

// the base ctor and one store.
RotationalPointResponsePhysics::RotationalPointResponsePhysics()
{
    m_refCount = 1;
}

RotationalPointResponsePhysics::~RotationalPointResponsePhysics() {}

void RotationalPointResponsePhysics::addRef()  { ++m_refCount; }
int  RotationalPointResponsePhysics::getRef()  { return m_refCount; }
int  RotationalPointResponsePhysics::release()
{
    const int r = --m_refCount;
    if (r == 0) delete this;
    return r;
}

void* RotationalPointResponsePhysics::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase)             return const_cast<RotationalPointResponsePhysics*>(this);
    if (iid == IID_IResponsePhysics)  return const_cast<RotationalPointResponsePhysics*>(this);
    return 0;
}

unsigned int RotationalPointResponsePhysics::getClassID() const          { return CID_RotationalPointResponsePhysics; }
void RotationalPointResponsePhysics::setCollisionChecked(unsigned int v) { m_collisionChecked = v; }
unsigned int RotationalPointResponsePhysics::getCollisionChecked() const { return m_collisionChecked; }

// Sweeps from the root's previous position to the point; the bool argument is unused.
// Requires valid objects/nodes, with only the other object's collision LOD gating the test.
// Projectiles try terrain before meshes and allow four hits/three slides plus a resting-contact arm;
// ordinary points perform one mesh query. Preserve port-specific getter order and discarded calls.
// Mesh rays take a direction, terrain rays an endpoint. The shared fraction output changes meaning
// between them, and contactSurfaceSpeed intentionally receives the other object's position.
void RotationalPointResponsePhysics::checkObjectVsObject(
    IObject* object, IObject* other, float dt, bool)
{
    typedef object_detail::LayoutAccess L;

    IPhysicsNode* const pointNode =
        static_cast<IPhysicsNode*>(L::physicsNode(object));               // first +0x44 read
    if (!other->hasCollisionLod(0))                                       // IObject slot 51
        return;

    ICollisionMesh* const otherMesh =
        static_cast<ICollisionMesh*>(L::collisionMesh(other));            // other +0x40
    ++collisionStatsData->unknown20;                                      // +0x20

    IPhysicsNode* const node =
        static_cast<IPhysicsNode*>(L::physicsNode(object));               // repeated +0x44 read
    IPhysicsNode* const otherNode = static_cast<IPhysicsNode*>(
        L::physicsNode(getRootParent(other)));                            // deliberately unguarded

    object->getAbsoluteTransformation();                                  // force/update; result unused

#if defined(_MSC_VER)
    // PE evaluates the right operand first; ELF evaluates the left operand first.
    const Vec3& otherLinearVelocity = otherNode->getLinearIntegrationVelocity();
    const Vec3& ownLinearVelocity = node->getLinearIntegrationVelocity();
#else
    const Vec3& ownLinearVelocity = node->getLinearIntegrationVelocity();
    const Vec3& otherLinearVelocity = otherNode->getLinearIntegrationVelocity();
#endif
    Vec3 direction;
    direction.x = (ownLinearVelocity.x - otherLinearVelocity.x) * dt;
    direction.y = (ownLinearVelocity.y - otherLinearVelocity.y) * dt;
    direction.z = (ownLinearVelocity.z - otherLinearVelocity.z) * dt;

#if defined(_MSC_VER)
    const Vec3& objectPosition = object->getAbsolutePosition();
    const Vec3& rootPosition = getRootParent(object)->getAbsolutePosition();
#else
    const Vec3& rootPosition = getRootParent(object)->getAbsolutePosition();
    const Vec3& objectPosition = object->getAbsolutePosition();
#endif
    direction.x += objectPosition.x - rootPosition.x;
    direction.y += objectPosition.y - rootPosition.y;
    direction.z += objectPosition.z - rootPosition.z;

    const Vec3& currentPosition = object->getAbsolutePosition();          // repeated, as shipped
    Vec3 rayStart;
    rayStart.x = currentPosition.x - direction.x;
    rayStart.y = currentPosition.y - direction.y;
    rayStart.z = currentPosition.z - direction.z;

    Vec3 hitNormal;
    hitNormal.x = hitNormal.y = hitNormal.z = 0.0f;
    Vec3 hitPosition;
    hitPosition.x = hitPosition.y = hitPosition.z = 0.0f;
    float distanceOrFraction;                                            // written by a successful query
    float alongDistance;                                                 // written by a successful mesh query
    int material = 0;
    bool anyHit = false;

    if (getObject()->getTemplate()->getClassID() != CID_GenericProjectileTemplate)
    {
        const Mat4& otherInverse = other->getInvAbsoluteTransformation(); // queried first
        const Mat4& otherTransform = other->getAbsoluteTransformation();
        if (!otherMesh->getDistance(0, false, otherTransform, otherInverse,
                                    rayStart, direction, hitNormal, hitPosition,
                                    distanceOrFraction, alongDistance, &material, 0,
                                    true, false))
            return;

        anyHit = true;
        object->setAbsolutePosition(hitPosition);

        if (hitNormal.y > 0.0f)
        {
            const Vec3& speedForGate = pointNode->getPositionalSpeed();
            if (speedForGate.y < 0.0f)
            {
                const float gravityAlongNormal =
                    static_cast<IPhysicsSystem*>(basicPhysicsSystem)->getGravity()
                    / 30.0f * hitNormal.y;
                float amountToRemove = speedForGate.y;
                if (amountToRemove <= gravityAlongNormal)
                    amountToRemove = gravityAlongNormal;

                Vec3 speed = pointNode->getPositionalSpeed();             // queried again
                speed.y += -amountToRemove;
                pointNode->setPositionalSpeed(speed);
            }
        }
    }
    else
    {
        const Vec3& acceleration = pointNode->getPositionalAcceleration();
        const float accelerationIntoOldNormal =
            acceleration.y * m_contactNormal.y
          + acceleration.x * m_contactNormal.x
          + acceleration.z * m_contactNormal.z;
        if (accelerationIntoOldNormal < 0.0f)
        {
            Vec3 speed = pointNode->getPositionalSpeed();
            const float speedIntoOldNormal =
                speed.x * m_contactNormal.x
              + speed.y * m_contactNormal.y
              + speed.z * m_contactNormal.z;
            const float restingThreshold =
                static_cast<IPhysicsSystem*>(basicPhysicsSystem)->getGravity()
                / 30.0f * 0.5f * 1.5f;
            if (speedIntoOldNormal > restingThreshold && speedIntoOldNormal < 0.0099999998f)
            {
                Vec3 gravityDirection;
                gravityDirection.x = 0.0f;
                gravityDirection.y =
                    (static_cast<IPhysicsSystem*>(basicPhysicsSystem)->getGravity() * 2.0f)
                    / 900.0f;
                gravityDirection.z = 0.0f;

#if defined(_MSC_VER)
                // The same source addition is evaluated in the opposite order by the two ports.
                const Vec3& centreOffsetForProbe = pointNode->getCenterOfCollisionOffset();
                const Vec3& positionForProbe = object->getAbsolutePosition();
#else
                const Vec3& positionForProbe = object->getAbsolutePosition();
                const Vec3& centreOffsetForProbe = pointNode->getCenterOfCollisionOffset();
#endif
                Vec3 probeStart;
                probeStart.x = positionForProbe.x + centreOffsetForProbe.x;
                probeStart.y = positionForProbe.y + centreOffsetForProbe.y;
                probeStart.z = positionForProbe.z + centreOffsetForProbe.z;

                const Mat4& otherInverse = other->getInvAbsoluteTransformation();
                const Mat4& otherTransform = other->getAbsoluteTransformation();
                if (otherMesh->getDistance(0, false, otherTransform, otherInverse,
                                           probeStart, gravityDirection, hitNormal, hitPosition,
                                           distanceOrFraction, alongDistance, &material, 0,
                                           true, false))
                {
                    hitPosition.y += s_objectCollisionSurfaceOffset;
                    const Vec3& centreOffset = pointNode->getCenterOfCollisionOffset();
                    Vec3 correctedPosition;
                    correctedPosition.x = hitPosition.x - centreOffset.x;
                    correctedPosition.y = hitPosition.y - centreOffset.y;
                    correctedPosition.z = hitPosition.z - centreOffset.z;
                    object->setAbsolutePosition(correctedPosition);

                    speed.x -= speedIntoOldNormal * m_contactNormal.x;
                    speed.y -= speedIntoOldNormal * m_contactNormal.y;
                    speed.z -= speedIntoOldNormal * m_contactNormal.z;

                    MaterialManager* const materials =
                        static_cast<MaterialManager*>(materialManager);
#if defined(_MSC_VER)
                    const float surfaceFriction =
                        materials->getFrictionForMaterial(m_contactMaterial);
                    const float ownFriction =
                        materials->getFrictionForMaterial(m_contactOwnMaterial);
#else
                    const float ownFriction =
                        materials->getFrictionForMaterial(m_contactOwnMaterial);
                    const float surfaceFriction =
                        materials->getFrictionForMaterial(m_contactMaterial);
#endif
                    float friction = (ownFriction + surfaceFriction) * 0.5f;
                    if (friction >= 1.0f)
                        friction = 1.0f;
                    const float grip = 1.0f - friction;
                    speed.x *= grip;
                    speed.y *= grip;
                    speed.z *= grip;
                    pointNode->setPositionalSpeed(speed);

                    const float oppositeGravity =
                        -static_cast<IPhysicsSystem*>(basicPhysicsSystem)->getGravity();
                    Vec3 contactAcceleration;
                    contactAcceleration.x = oppositeGravity * m_contactNormal.x;
                    contactAcceleration.y = oppositeGravity * m_contactNormal.y;
                    contactAcceleration.z = oppositeGravity * m_contactNormal.z;
                    pointNode->addAccelerationAtRelativePosition(
                        Vec3::getZero(), contactAcceleration);
                    return;
                }
            }
        }

        Vec3 adjustedHit;
        adjustedHit.x = adjustedHit.y = adjustedHit.z = 0.0f;
        const Vec3& centreOffset = pointNode->getCenterOfCollisionOffset();
        Vec3 currentStart;
        currentStart.x = rayStart.x + centreOffset.x;
        currentStart.y = rayStart.y + centreOffset.y;
        currentStart.z = rayStart.z + centreOffset.z;
        Vec3 currentDirection = direction;
        const float originalLength = sqrtf(direction.z * direction.z
                                         + direction.y * direction.y
                                         + direction.x * direction.x);
        bool meshPhase = false;
        int iterationsLeft = s_objectCollisionIterations + 1;

        for (;;)
        {
            bool hit;
            if (meshPhase)
            {
                const Mat4& otherInverse = other->getInvAbsoluteTransformation();
                const Mat4& otherTransform = other->getAbsoluteTransformation();
                hit = otherMesh->getDistance(0, false, otherTransform, otherInverse,
                                             currentStart, currentDirection,
                                             hitNormal, hitPosition,
                                             distanceOrFraction, alongDistance, &material, 0,
                                             true, false);
            }
            else
            {
                unsigned char terrainMaterial = 0;
                Vec3 rayEnd;
                rayEnd.x = currentStart.x + currentDirection.x;
                rayEnd.y = currentStart.y + currentDirection.y;
                rayEnd.z = currentStart.z + currentDirection.z;
                hit = static_cast<IHeightmapCluster*>(heightmapCluster)
                    ->intersectRayInWorldCoords(currentStart, rayEnd, hitPosition,
                                                distanceOrFraction, hitNormal,
                                                terrainMaterial, false);
                material = terrainMaterial;
            }

            if (hit)
            {
                anyHit = true;
                Vec3 normalDirection = currentDirection;
                normalizeObjectCollisionDirection(normalDirection);

                --iterationsLeft;
                adjustedHit.x = hitPosition.x
                              - s_objectCollisionSurfaceOffset * normalDirection.x;
                adjustedHit.y = hitPosition.y
                              - s_objectCollisionSurfaceOffset * normalDirection.y;
                adjustedHit.z = hitPosition.z
                              - s_objectCollisionSurfaceOffset * normalDirection.z;
                if (iterationsLeft == 0)
                    break;

                Vec3 remaining;
                remaining.x = currentStart.x + currentDirection.x - adjustedHit.x;
                remaining.y = currentStart.y + currentDirection.y - adjustedHit.y;
                remaining.z = currentStart.z + currentDirection.z - adjustedHit.z;
                const float intoNormal = hitNormal.y * remaining.y
                                       + hitNormal.x * remaining.x
                                       + hitNormal.z * remaining.z;
                currentDirection.x = remaining.x - hitNormal.x * intoNormal;
                currentDirection.y = remaining.y - hitNormal.y * intoNormal;
                currentDirection.z = remaining.z - hitNormal.z * intoNormal;

                const float newLength = sqrtf(currentDirection.y * currentDirection.y
                                            + currentDirection.x * currentDirection.x
                                            + currentDirection.z * currentDirection.z);
                if (s_objectCollisionMinDirectionRatio > newLength / originalLength)
                    break;                                                   // no zero-length guard

                currentStart = adjustedHit;
                meshPhase = true;
                continue;
            }

            if (!meshPhase)
            {
                meshPhase = true;                                           // terrain miss -> mesh
                continue;
            }
            if (!anyHit)
                return;
            break;
        }

        const Vec3& finalOffset = pointNode->getCenterOfCollisionOffset();
        Vec3 correctedPosition;
        correctedPosition.x = adjustedHit.x - finalOffset.x;
        correctedPosition.y = adjustedHit.y - finalOffset.y;
        correctedPosition.z = adjustedHit.z - finalOffset.z;
        object->setAbsolutePosition(correctedPosition);
    }

    if (!anyHit)
        return;

    const Vec3& positionAtContact = object->getAbsolutePosition();
    const float dx = hitPosition.x - positionAtContact.x;
    const float dy = hitPosition.y - positionAtContact.y;
    const float dz = hitPosition.z - positionAtContact.z;
    const float depthSq = dz * dz + dy * dy + dx * dx;
    if (!(depthSq > m_contactDepthSq))
        return;

    m_contactDepthSq = depthSq;
    m_contactObject = other;
    m_contactPoint = hitPosition;
    m_contactPosition = object->getAbsolutePosition();                     // queried again
    m_contactSurfaceSpeed = other->getAbsolutePosition();                  // yes: position, not speed

#if defined(_MSC_VER)
    const Vec3& otherTangentSpeed = otherNode->getTangentSpeed(hitPosition);
    const Vec3& ownSpeed = node->getPositionalSpeed();
#else
    const Vec3& ownSpeed = node->getPositionalSpeed();
    const Vec3& otherTangentSpeed = otherNode->getTangentSpeed(hitPosition);
#endif
    m_contactSpeed.x = ownSpeed.x - otherTangentSpeed.x;
    m_contactSpeed.y = ownSpeed.y - otherTangentSpeed.y;
    m_contactSpeed.z = ownSpeed.z - otherTangentSpeed.z;
    if (fabsf(m_contactSpeed.x) < 0.000001f) m_contactSpeed.x = 0.0f;
    if (fabsf(m_contactSpeed.y) < 0.000001f) m_contactSpeed.y = 0.0f;
    if (fabsf(m_contactSpeed.z) < 0.000001f) m_contactSpeed.z = 0.0f;
    m_contactScale = 1.0f;
    m_contactFraction = distanceOrFraction;
    m_contactNormal = hitNormal;
    m_contactOwnMaterial = 0;
    m_contactMaterial = material;
}

// Samples terrain beneath the body's lowest point, retaining the deepest non-positive depth.
// Collision-group bit 0 opts out; m_enabled does not gate this test. Water can replace the sampled
// surface when the node is not already submerged. Stores depth in the contact-fraction field and
// snaps tiny speed components to zero. Water contacts nudge by 1 mm; land updates node submersion.
void RotationalPointResponsePhysics::checkVsTerrain(float dt)
{
    typedef object_detail::LayoutAccess L;

    if ((getCollisionGroups() & 1) != 0)                                  // +0x54
        return;
    ++collisionStatsData->unknown24;                                      // +0x24

    IObject* const object = m_object;
    const Vec3& position = object->getAbsolutePosition();                 // slot 27
    IPhysicsNode* const node = static_cast<IPhysicsNode*>(L::physicsNode(object));   // +0x44
    const Vec3& velocity = node->getLinearIntegrationVelocity();          // +0x16C
    const float sweepY = dt * velocity.y;

    Vec3 normal;
    normal.x = normal.y = normal.z = 0.0f;
    unsigned char material = 0;
    IHeightmapCluster* const cluster = static_cast<IHeightmapCluster*>(heightmapCluster);

    const float lowest = node->getCenterOfCollisionOffset().y + position.y;   // +0xEC
    float depth = lowest
        - cluster->getHeightAndNormalInWorldCoords(position.x, position.z, normal, material);   // slot 33
    if (depth > 0.0f && node->getUnderWater() <= 0.0f)                    // +0x10C
    {
        const float waterDepth = lowest - cluster->getWaterLevelInWorldCoords(position);   // slot 39
        if (waterDepth <= 0.0f)
        {
            depth = waterDepth;
            normal.x = 0.0f;
            normal.y = 1.0f;
            normal.z = 0.0f;
            material = 1;
        }
    }
    if (!(depth <= 0.0f))
        return;

    const float depthSq = depth * depth;
    if (!(depthSq > m_contactDepthSq))
        return;

    m_contactDepthSq = depthSq;
    m_contactObject = 0;
    m_contactPoint = position;
    m_contactPoint.y = m_contactPoint.y - depth;
    const Vec3& offset = node->getCenterOfCollisionOffset();
    m_contactPoint.x = m_contactPoint.x - offset.x;
    m_contactPoint.y = m_contactPoint.y - offset.y;
    m_contactPoint.z = m_contactPoint.z - offset.z;
    m_contactPosition = position;
    m_contactSurfaceSpeed.x = 0.0f;
    m_contactSurfaceSpeed.y = 0.0f;
    m_contactSurfaceSpeed.z = 0.0f;
    m_contactSpeed = node->getPositionalSpeed();                          // +0x60
    if (fabsf(m_contactSpeed.x) < 0.000001f) m_contactSpeed.x = 0.0f;
    if (fabsf(m_contactSpeed.y) < 0.000001f) m_contactSpeed.y = 0.0f;
    if (fabsf(m_contactSpeed.z) < 0.000001f) m_contactSpeed.z = 0.0f;
    m_contactScale = 1.0f;
    m_contactFraction = depth;
    m_contactNormal = normal;
    m_contactOwnMaterial = 0;
    m_contactMaterial = material;

    if (material == 1)                                                    // the water surface
    {
        if (0.0f < sweepY)
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

    const float below = m_contactPoint.y - cluster->getWaterLevelInWorldCoords(m_contactPoint);
    if (below >= 0.0f)
        m_underWaterDepth = 0.0f;
    else
        m_underWaterDepth = -below;
    node->setUnderWater(m_underWaterDepth);                               // +0x108
}

// Resolves a recorded tumbling-point contact without an enabled-state gate, then clears it.
// Uses the object's material and no face info. Nonwater contacts push out and apply elasticity/friction
// to velocity; angular velocity reverses and scales with resulting speed.
void RotationalPointResponsePhysics::solveImpulse(IPhysicsNode* node)
{
    if (!(0.0f <= m_contactDepthSq))
        return;

    m_contactOwnMaterial = m_object->getMaterial();                       // slot 52
    Vec3 penetration;
    penetration.x = m_contactPoint.x - m_contactPosition.x;
    penetration.y = m_contactPoint.y - m_contactPosition.y;
    penetration.z = m_contactPoint.z - m_contactPosition.z;
    getObject()->handleCollision(m_contactObject, m_contactSpeed, m_contactNormal, penetration,
                                 m_contactOwnMaterial, m_contactMaterial, 0, 0);   // slot 45
    node->setUnderWater(m_underWaterDepth);                               // +0x108

    MaterialManager* const materials = static_cast<MaterialManager*>(materialManager);
    Material* const surface = materials->getMaterialPtr(static_cast<unsigned int>(m_contactMaterial));   // +0x24
    if (!surface->getHasWaterPhysics())                                   // slot 25
    {
        const Vec3& position = node->getAbsolutePosition();               // +0x48
        const float depth = m_contactFraction;
        Vec3 pushed;
        pushed.x = position.x - depth * m_contactNormal.x;
        pushed.y = position.y - depth * m_contactNormal.y;
        pushed.z = position.z - depth * m_contactNormal.z;
        node->setAbsolutePosition(pushed);                                // +0x4C

        const float into = -m_contactSpeed.y * m_contactNormal.y
                         - m_contactSpeed.x * m_contactNormal.x
                         - m_contactSpeed.z * m_contactNormal.z;
        Vec3 normalPart;
        normalPart.x = m_contactNormal.x * into;
        normalPart.y = m_contactNormal.y * into;
        normalPart.z = m_contactNormal.z * into;
        Vec3 tangent;
        tangent.x = normalPart.x + m_contactSpeed.x;
        tangent.y = normalPart.y + m_contactSpeed.y;
        tangent.z = normalPart.z + m_contactSpeed.z;

        const float ownElasticity = materials->getElasticityForMaterial(m_contactOwnMaterial);   // +0x70
        const float elasticity =
            0.5f * (materials->getElasticityForMaterial(m_contactMaterial) + ownElasticity);
        const float ownFriction = materials->getFrictionForMaterial(m_contactOwnMaterial);     // +0x6C
        float friction = (materials->getFrictionForMaterial(m_contactMaterial) + ownFriction) * 0.5f;
        if (friction >= 1.0f)
            friction = 1.0f;
        const float grip = 1.0f - friction;

        Vec3 newSpeed;
        newSpeed.x = elasticity * normalPart.x + tangent.x * grip;
        newSpeed.y = elasticity * normalPart.y + tangent.y * grip;
        newSpeed.z = elasticity * normalPart.z + grip * tangent.z;
        node->setPositionalSpeed(newSpeed);                               // +0x68

        const Vec3& rotation = node->getRotationalSpeed();                // +0x64
        float s = newSpeed.z * newSpeed.z + newSpeed.y * newSpeed.y + newSpeed.x * newSpeed.x;
        if (1.0f < s)
            s = 1.0f;
        const float factor = s >= 0.1f ? s : 0.0f;
        Vec3 newRotation;
        newRotation.x = rotation.x * -factor;
        newRotation.y = rotation.y * -factor;
        newRotation.z = rotation.z * -factor;
        node->setRotationalSpeed(newRotation);                            // +0x6C
    }

    m_contactDepthSq = -1.0f;
}

} // namespace world
} // namespace hfe
} // namespace dice
