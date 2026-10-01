// Rotational point-node integration and orientation updates; source path is attested.

#include "RotationalPointPhysicsNode.h"

#include <math.h>

#include "BasicPhysicsSystem.h"
#include "../Core/Debug.h"
#include "../Core/Ids.h"
#include "../Math/Mat4.h"

namespace dice {
namespace hfe {
namespace world {

static const float EPS = 0.000001f;

static inline void snapSmall(Vec3& v)
{
    if (fabsf(v.x) < EPS) v.x = 0.0f;
    if (fabsf(v.y) < EPS) v.y = 0.0f;
    if (fabsf(v.z) < EPS) v.z = 0.0f;
}

static inline void zero(Vec3& v) { v.x = v.y = v.z = 0.0f; }

static inline IPhysicsSystem* physicsSystem()
{
    return static_cast<IPhysicsSystem*>(basicPhysicsSystem);
}

// PointPhysicsNode ctor, the counter, twenty zeroed dwords, then setIsAwake()
// (the base's - a virtual call in the ELF, resolved statically inside a constructor).
RotationalPointPhysicsNode::RotationalPointPhysicsNode()
{
    m_refCount = 1;
    zero(m_rotationalSpeed);
    zero(m_angularIntegrationVelocity);
    zero(m_rotationalAcceleration);
    zero(m_positionalFriction);
    zero(m_rotationalFriction);
    m_dwordD0 = 0;
    zero(m_centerOfCollisionOffset);
    setIsAwake();
}

RotationalPointPhysicsNode::~RotationalPointPhysicsNode() {}

void RotationalPointPhysicsNode::addRef()  { ++m_refCount; }
int  RotationalPointPhysicsNode::getRef()  { return m_refCount; }
int  RotationalPointPhysicsNode::release()
{
    const int r = --m_refCount;
    if (r == 0) delete this;
    return r;
}

void* RotationalPointPhysicsNode::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase)         return const_cast<RotationalPointPhysicsNode*>(this);
    if (iid == IID_IPhysicsNode)  return const_cast<RotationalPointPhysicsNode*>(this);
    return 0;
}

const Vec3& RotationalPointPhysicsNode::getRotationalSpeed() const { return m_rotationalSpeed; }

void RotationalPointPhysicsNode::setRotationalSpeed(const Vec3& v)
{
    m_rotationalSpeed = v;
    snapSmall(m_rotationalSpeed);
}

void RotationalPointPhysicsNode::addRotationalSpeed(const Vec3& v)
{
    m_rotationalSpeed.x += v.x; m_rotationalSpeed.y += v.y; m_rotationalSpeed.z += v.z;
    snapSmall(m_rotationalSpeed);
}

void RotationalPointPhysicsNode::setRotationalAcceleration(const Vec3& v)
{
    m_rotationalAcceleration = v;
    snapSmall(m_rotationalAcceleration);
}

void RotationalPointPhysicsNode::setPositionalFriction(const Vec3& v)
{
    m_positionalFriction = v;
    snapSmall(m_positionalFriction);
}

void RotationalPointPhysicsNode::setRotationalFriction(const Vec3& v)
{
    m_rotationalFriction = v;
    snapSmall(m_rotationalFriction);
}

const Vec3& RotationalPointPhysicsNode::getRotationalAcceleration() const { return m_rotationalAcceleration; }
const Vec3& RotationalPointPhysicsNode::getPositionalFriction() const     { return m_positionalFriction; }
const Vec3& RotationalPointPhysicsNode::getRotationalFriction() const     { return m_rotationalFriction; }

// see the header. The Debug is type 6: process-fatal, as shipped.
void RotationalPointPhysicsNode::updatePhysics(float dt)
{
    if (!m_enabled)
        return;
    if (m_drag > 0.0f)
    {
        if (m_mass < 0.0000099999997f)
            Debug(DEBUGTYPE_6, "Physics/RotationalPointPhysicsNode.cpp", 58, __FUNCTION__)
                << "Mass 0 when calculating drag.";
        const float radius = getBoundingRadius();     // virtual +0x38 -> the object's
        float u = m_underWater / radius;
        if (u > 1.0f)
            u = 1.0f;
        const float dragMod = 1.0f - u + u * 25.0f;
        updatePositionalDragSimple(radius, dragMod);
    }
    PointPhysicsNode::updatePositionalPhysics(dt);
    updateRotationalPhysics(dt);
    m_positionalAcceleration.y = physicsSystem()->getGravity() * m_gravityModifier
                               + m_positionalAcceleration.y;
}

void RotationalPointPhysicsNode::setCenterOfCollisionOffset(const Vec3& v)   { m_centerOfCollisionOffset = v; }
const Vec3& RotationalPointPhysicsNode::getCenterOfCollisionOffset() const   { return m_centerOfCollisionOffset; }

// the base reset, then +0x94..+0xD3 zeroed (not the collision offset).
void RotationalPointPhysicsNode::reset(bool full)
{
    PointPhysicsNode::reset(full);
    zero(m_rotationalSpeed);
    zero(m_angularIntegrationVelocity);
    zero(m_rotationalAcceleration);
    zero(m_positionalFriction);
    zero(m_rotationalFriction);
    m_dwordD0 = 0;
}

// this class's positional drag - written straight into the acceleration (no
// virtual adder, no snap): acc -= pi * r^2 * drag / mass * (speed * mod - wind).
void RotationalPointPhysicsNode::updatePositionalDragSimple(float radius, float dragMod)
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

// the angular twin, no wind.
void RotationalPointPhysicsNode::updateRotationalDragSimple(float radius, float dragMod)
{
    if (!m_enabled)
        return;
    const float k = radius * (radius * 3.1415927f) * m_drag * dragMod;
    const float invMass = 1.0f / m_mass;
    m_rotationalAcceleration.x = m_rotationalAcceleration.x - (m_rotationalSpeed.x * k) * invMass;
    m_rotationalAcceleration.y = m_rotationalAcceleration.y - (m_rotationalSpeed.y * k) * invMass;
    m_rotationalAcceleration.z = m_rotationalAcceleration.z - (m_rotationalSpeed.z * k) * invMass;
}

// / The rotational point is treated as a sphere with
// radius 0.1: I = 2/3 * mass * r^2. Its acceleration is capped at length 1,000, but an integration
// delta over that limit is discarded outright. The object's const transformation reference is
// then rotated in place, matching both retail ports.
void RotationalPointPhysicsNode::updateRotationalPhysics(float dt)
{
    if (!m_enabled)
        return;

    const float accelerationSq = m_rotationalAcceleration.y * m_rotationalAcceleration.y
                               + m_rotationalAcceleration.x * m_rotationalAcceleration.x
                               + m_rotationalAcceleration.z * m_rotationalAcceleration.z;
    if (accelerationSq > 1000000.0f)
    {
        const float scale = 1000.0f / sqrtf(accelerationSq);
        m_rotationalAcceleration.x *= scale;
        m_rotationalAcceleration.y *= scale;
        m_rotationalAcceleration.z *= scale;
    }

    Vec3 delta;
    const float inertia = (0.66666669f * m_mass) * 0.1f * 0.1f;
    const float scale = dt / inertia;
    delta.x = (m_rotationalAcceleration.x + m_rotationalFriction.x) * scale;
    delta.y = (m_rotationalAcceleration.y + m_rotationalFriction.y) * scale;
    delta.z = (m_rotationalAcceleration.z + m_rotationalFriction.z) * scale;

    if (delta.z * delta.z + delta.y * delta.y + delta.x * delta.x > 1000000.0f)
        zero(delta);

    const Vec3 previous = m_rotationalSpeed;
    addRotationalSpeed(delta);

    m_angularIntegrationVelocity.x = (m_rotationalSpeed.x + previous.x) * 0.5f;
    m_angularIntegrationVelocity.y = (m_rotationalSpeed.y + previous.y) * 0.5f;
    m_angularIntegrationVelocity.z = (m_rotationalSpeed.z + previous.z) * 0.5f;
    snapSmall(m_angularIntegrationVelocity);

    const float angularSpeedSq =
        m_angularIntegrationVelocity.y * m_angularIntegrationVelocity.y
      + m_angularIntegrationVelocity.x * m_angularIntegrationVelocity.x
      + m_angularIntegrationVelocity.z * m_angularIntegrationVelocity.z;
    if (angularSpeedSq > 0.0000010000001f)
    {
        const float angularSpeed = sqrtf(angularSpeedSq);
        const float inverseAngularSpeed = 1.0f / angularSpeed;
        Vec3 axis;
        axis.x = m_angularIntegrationVelocity.x * inverseAngularSpeed;
        axis.y = m_angularIntegrationVelocity.y * inverseAngularSpeed;
        axis.z = m_angularIntegrationVelocity.z * inverseAngularSpeed;

        Mat4& transformation = const_cast<Mat4&>(getAbsoluteTransformation());
        const float degrees = angularSpeed * dt * 57.295776f;
        if (fabsf(degrees) > 1.19209290e-7f)
        {
            Mat4 rotation = Mat4::getIdentity();
            setRotateAboutLine(rotation, degrees, axis);
            const Mat4 before = transformation;
            transformation.rotMult(before, rotation);
        }
        setAbsoluteTransformation(transformation);
    }

    zero(m_rotationalAcceleration);
    zero(m_rotationalFriction);
    m_dwordD0 = 0;
}

} // namespace world
} // namespace hfe
} // namespace dice
