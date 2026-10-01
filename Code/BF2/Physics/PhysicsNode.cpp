// Rigid-body physics integration, impulses, drag, and latency-history queries.

#include "PhysicsNode.h"

#include <math.h>

#include "../Core/Ids.h"
#include "../Game/Objects/IObject.h"
#include "../Game/Objects/ObjectLayoutAccess.h"
#include "../Game/Objects/IObjectTemplate.h"          // getClassID / queryImplementation (updatePhysics)
#include "../Math/LocalBoundingBox.h"                  // the lod-1 box (drag, gravity compensation)
#include "BasicPhysicsSystem.h"                        // basicPhysicsSystem - getGravity / getWind
#include "ICollisionMesh.h"                            // hasLod / getBoundingBox
#include "../Core/Debug.h"                             // getInverseInertia's abort
#include "../Game/Objects/Base/SimpleObjectTemplate.h" // m_floaterMod (+0x88)
#include "../Game/Objects/Fx/ParticleTemplate.h"       // m_floaterMod (+0xA0)
#include "../Math/Mat4.h"                              // setRotateAboutLine / rotMult
#include "ResponsePhysics.h"                           // the +0xF4 resting-on block (the sleeping arm)

namespace dice {
namespace hfe {
namespace world {

extern IBase* basicPhysicsSystem;                             // ds:0x8B2784C - Core/Globals.cpp

typedef object_detail::LayoutAccess L;

static const float EPS = 0.000001f;

static inline void snapSmall(Vec3& v)
{
    if (fabsf(v.x) < EPS) v.x = 0.0f;
    if (fabsf(v.y) < EPS) v.y = 0.0f;
    if (fabsf(v.z) < EPS) v.z = 0.0f;
}

static inline void zero(Vec3& v) { v.x = v.y = v.z = 0.0f; }

// The two advanced-drag bodies carry three inlined copies of this projection. They deliberately
// divide by |axis|^2 instead of assuming an orthonormal object transformation.
static inline Vec3 projectOntoAxis(const Vec3& value, const Vec3& axis)
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

// Cap a one-frame drag step so it cannot pass the current speed on the way to zero. The ordered
// branches are kept instead of std::min/max because their NaN behaviour is part of the x87 code.
static inline float clipDragComponent(float speed, float drag)
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

static inline Vec3 rotateVector(const Mat4& transformation, const Vec3& value)
{
    Vec3 result = {
        transformation.m[0] * value.x + transformation.m[4] * value.y
            + transformation.m[8] * value.z,
        transformation.m[1] * value.x + transformation.m[5] * value.y
            + transformation.m[9] * value.z,
        transformation.m[2] * value.x + transformation.m[6] * value.y
            + transformation.m[10] * value.z
    };
    return result;
}

// PointPhysicsNode ctor, then every field (raw dword stores and three small
// zero-loops for the Vec3 arrays and the identity matrix), otherCollisionLod = 1, setIsAwake().
PhysicsNode::PhysicsNode()
{
    m_refCount = 1;
    zero(m_rotationalSpeed);
    zero(m_rotationalAcceleration);
    zero(m_localAngularSpeed);
    zero(m_angularIntegrationVelocity);
    zero(m_positionalFriction);
    zero(m_rotationalFriction);
    zero(m_dragOffset);
    zero(m_dragModifier);
    m_dwordF4 = 0;
    zero(m_centerOfMassOffset);
    zero(m_centerOfCollisionOffset);
    m_inertiaModifier.x = m_inertiaModifier.y = m_inertiaModifier.z = 1.0f;
    m_fixToParent = true;
    m_latencyPositions = 0;
    m_latencyRotations = 0;
    m_latencyIndex = 0;
    zero(m_vec12C);
    zero(m_vec138);
    m_byte144 = false;
    zero(m_oldPosition);
    zero(m_vec154);
    for (int i = 0; i < 5; ++i) zero(m_vec160[i]);
    m_dword19C = 0;
    zero(m_vec1A0);
    for (int i = 0; i < 3; ++i) zero(m_vec1AC[i]);
    m_dword1D0 = 0;
    m_prevTransformation = Mat4::getIdentity();
    m_prevTransformationIsObjects = true;
    m_isPlatformPhysics = false;
    m_inverseInertiaValid = false;
    zero(m_inverseInertia);
    m_otherCollisionLod = 1;                         // +0x8C, over the base's 0
    m_gravityCompensateOnContact = -1;
    m_hasContact = false;
    m_isInAir = true;
    setIsAwake();
}

// the two rings, then the base.
PhysicsNode::~PhysicsNode()
{
    if (m_latencyPositions)
        delete[] m_latencyPositions;
    if (m_latencyRotations)
        delete[] m_latencyRotations;
}

void PhysicsNode::addRef()  { ++m_refCount; }
int  PhysicsNode::getRef()  { return m_refCount; }
int  PhysicsNode::release()
{
    const int r = --m_refCount;
    if (r == 0) delete this;
    return r;
}

void* PhysicsNode::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase)         return const_cast<PhysicsNode*>(this);
    if (iid == IID_IPhysicsNode)  return const_cast<PhysicsNode*>(this);
    return 0;
}

// the object tree's parent, asked for its node.
IPhysicsNode* PhysicsNode::getParent() const
{
    IObject* p = L::parent(m_object);
    return p ? static_cast<IPhysicsNode*>(L::physicsNode(p)) : 0;
}

void PhysicsNode::setAbsoluteTransformation(const Mat4& m) { m_object->setAbsoluteTransformation(m); }
void PhysicsNode::setAbsolutePosition(const Vec3& p)       { m_object->setAbsolutePosition(p); }


const Mat4& PhysicsNode::getPrevTransformation() const
{
    if (m_prevTransformationIsObjects)
        return m_object->getAbsoluteTransformation();
    return m_prevTransformation;
}

// clear the flag, copy the sixteen floats.
void PhysicsNode::setPrevTransformation(const Mat4& m)
{
    m_prevTransformationIsObjects = false;
    m_prevTransformation = m;
}

void PhysicsNode::setOldPosition(const Vec3& p) { m_oldPosition = p; }

// a parented body reports no speed of its own.
const Vec3& PhysicsNode::getPositionalSpeed() const
{
    if (L::parent(m_object))
        return Vec3::getZero();
    return m_positionalSpeed;
}

const Vec3& PhysicsNode::getRotationalSpeed() const
{
    if (L::parent(m_object))
        return Vec3::getZero();
    return m_rotationalSpeed;
}

void PhysicsNode::setRotationalSpeed(const Vec3& v)
{
    m_rotationalSpeed = v;
    snapSmall(m_rotationalSpeed);
}

void PhysicsNode::addRotationalSpeed(const Vec3& v)
{
    m_rotationalSpeed.x += v.x; m_rotationalSpeed.y += v.y; m_rotationalSpeed.z += v.z;
    snapSmall(m_rotationalSpeed);
}

void PhysicsNode::setRotationalAcceleration(const Vec3& v)
{
    m_rotationalAcceleration = v;
    snapSmall(m_rotationalAcceleration);
}

void PhysicsNode::setPositionalFriction(const Vec3& v)
{
    m_positionalFriction = v;
    snapSmall(m_positionalFriction);
}

void PhysicsNode::setRotationalFriction(const Vec3& v)
{
    m_rotationalFriction = v;
    snapSmall(m_rotationalFriction);
}

// addSpeedAtAbsolutePosition - / A speed impulse at a
// world-space point contributes its vector to linear speed and (point - body position) x speed to
// angular speed. A fixed child forwards both original arguments to its parent. Both ports repeat
// the virtual getParent call in that arm; that observable ordering is kept.
void PhysicsNode::addSpeedAtAbsolutePosition(const Vec3& absolutePosition, const Vec3& speed)
{
    if (!m_enabled)
        return;

    if (getParent() && m_fixToParent)
    {
        getParent()->addSpeedAtAbsolutePosition(absolutePosition, speed);
        return;
    }

    addPositionalSpeed(speed);

    const Vec3& origin = getAbsolutePosition();
    const float rx = absolutePosition.x - origin.x;
    const float ry = absolutePosition.y - origin.y;
    const float rz = absolutePosition.z - origin.z;
    Vec3 moment;
    moment.x = ry * speed.z - rz * speed.y;
    moment.y = rz * speed.x - rx * speed.z;
    moment.z = rx * speed.y - ry * speed.x;
    addRotationalSpeed(moment);
}

// addSpeedAtRelativePosition - / The relative-position
// twin above: the supplied position already is the lever arm, so no body-position subtraction is
// made. The fixed-child forwarding and repeated parent lookup are identical in both ports.
void PhysicsNode::addSpeedAtRelativePosition(const Vec3& relativePosition, const Vec3& speed)
{
    if (!m_enabled)
        return;

    if (getParent() && m_fixToParent)
    {
        getParent()->addSpeedAtRelativePosition(relativePosition, speed);
        return;
    }

    addPositionalSpeed(speed);

    Vec3 moment;
    moment.x = relativePosition.y * speed.z - relativePosition.z * speed.y;
    moment.y = relativePosition.z * speed.x - relativePosition.x * speed.z;
    moment.z = relativePosition.x * speed.y - relativePosition.y * speed.x;
    addRotationalSpeed(moment);
}

// Adds linear acceleration and its moment about the center of mass; disabled nodes ignore it.
// A fixed child forwards to its parent with the relative offset unchanged.
void PhysicsNode::addAccelerationAtRelativePosition(const Vec3& relativePosition, const Vec3& acceleration)
{
    if (!m_enabled)                                                                  // byte +0x0C
        return;

    IPhysicsNode* parent = getParent();                                              // vslot +0x34
    if (parent && m_fixToParent)                                                     // byte +0x11C
    {
        parent->addAccelerationAtRelativePosition(relativePosition, acceleration);   // +0x94
        return;
    }

    m_positionalAcceleration.x += acceleration.x;                                    // +0x2C
    m_positionalAcceleration.y += acceleration.y;
    m_positionalAcceleration.z += acceleration.z;
    snapSmall(m_positionalAcceleration);

    // The lever arm is measured from the centre of mass, not from the node's origin.
    const float rx = relativePosition.x - m_centerOfMassOffset.x;                    // +0xF8
    const float ry = relativePosition.y - m_centerOfMassOffset.y;
    const float rz = relativePosition.z - m_centerOfMassOffset.z;

    m_rotationalAcceleration.x += ry * acceleration.z - rz * acceleration.y;         // +0xA0
    m_rotationalAcceleration.y += rz * acceleration.x - rx * acceleration.z;
    m_rotationalAcceleration.z += rx * acceleration.y - ry * acceleration.x;
    snapSmall(m_rotationalAcceleration);
}

// Adds acceleration at an absolute position, measuring its lever arm from the node
// position and center of mass.
void PhysicsNode::addAccelerationAtAbsolutePosition(const Vec3& absolutePosition, const Vec3& acceleration)
{
    if (!m_enabled)                                                                  // byte +0x0C
        return;

    IPhysicsNode* parent = getParent();                                              // vslot +0x34
    if (parent && m_fixToParent)                                                     // byte +0x11C
    {
        parent->addAccelerationAtAbsolutePosition(absolutePosition, acceleration);   // +0x90
        return;
    }

    m_positionalAcceleration.x += acceleration.x;                                    // +0x2C
    m_positionalAcceleration.y += acceleration.y;
    m_positionalAcceleration.z += acceleration.z;
    snapSmall(m_positionalAcceleration);

    const Vec3& here = getAbsolutePosition();                                        // +0x48
    const float rx = absolutePosition.x - here.x - m_centerOfMassOffset.x;           // +0xF8
    const float ry = absolutePosition.y - here.y - m_centerOfMassOffset.y;
    const float rz = absolutePosition.z - here.z - m_centerOfMassOffset.z;

    m_rotationalAcceleration.x += ry * acceleration.z - rz * acceleration.y;         // +0xA0
    m_rotationalAcceleration.y += rz * acceleration.x - rx * acceleration.z;
    m_rotationalAcceleration.z += rx * acceleration.y - ry * acceleration.x;
    snapSmall(m_rotationalAcceleration);
}

// addPositionalAccelerationToRootParent - / Recursively
// forwards through fixed parents; the first independent body adds the vector to its positional
// acceleration and performs the family's per-component epsilon snap. Both ports query the parent
// twice in the forwarding arm.
void PhysicsNode::addPositionalAccelerationToRootParent(const Vec3& acceleration)
{
    if (!m_enabled)
        return;

    if (getParent() && m_fixToParent)
    {
        getParent()->addPositionalAccelerationToRootParent(acceleration);
        return;
    }

    m_positionalAcceleration.x += acceleration.x;
    m_positionalAcceleration.y += acceleration.y;
    m_positionalAcceleration.z += acceleration.z;
    snapSmall(m_positionalAcceleration);
}

// addRotationalAccelerationAtAbsolutePosition - / Adds
// only the moment of the supplied acceleration about the centre of mass; it deliberately does not
// add the acceleration to the body's linear component. Fixed children forward the absolute point
// and vector unchanged, with the shipped repeated parent lookup.
void PhysicsNode::addRotationalAccelerationAtAbsolutePosition(const Vec3& absolutePosition,
                                                               const Vec3& acceleration)
{
    if (!m_enabled)
        return;

    if (getParent() && m_fixToParent)
    {
        getParent()->addRotationalAccelerationAtAbsolutePosition(absolutePosition, acceleration);
        return;
    }

    const Vec3& origin = getAbsolutePosition();
    const float rx = absolutePosition.x - origin.x - m_centerOfMassOffset.x;
    const float ry = absolutePosition.y - origin.y - m_centerOfMassOffset.y;
    const float rz = absolutePosition.z - origin.z - m_centerOfMassOffset.z;
    m_rotationalAcceleration.x += ry * acceleration.z - rz * acceleration.y;
    m_rotationalAcceleration.y += rz * acceleration.x - rx * acceleration.z;
    m_rotationalAcceleration.z += rx * acceleration.y - ry * acceleration.x;
    snapSmall(m_rotationalAcceleration);
}

// Accumulates a local impulse and position-cross-impulse moment, snapping components
// below 1e-6. Disabled nodes ignore it; parent-fixed nodes forward unchanged.
// The lever arm uses relativePosition directly, without a center-of-mass adjustment.
void PhysicsNode::addRelativeImpulse(const Vec3& relativePosition, const Vec3& impulse)
{
    if (!m_enabled)                                                                  // byte +0x0C
        return;

    IPhysicsNode* parent = getParent();                                              // vslot +0x34
    if (parent && m_fixToParent)                                                     // byte +0x11C
    {
        parent->addRelativeImpulse(relativePosition, impulse);                       // +0x160
        return;
    }

    m_localLinearSpeed.x = impulse.x + m_localLinearSpeed.x;                         // +0x38
    m_localLinearSpeed.y = impulse.y + m_localLinearSpeed.y;
    m_localLinearSpeed.z = impulse.z + m_localLinearSpeed.z;
    snapSmall(m_localLinearSpeed);

    m_localAngularSpeed.x = impulse.z * relativePosition.y - impulse.y * relativePosition.z
                          + m_localAngularSpeed.x;                                   // +0xAC
    m_localAngularSpeed.y = relativePosition.z * impulse.x - relativePosition.x * impulse.z
                          + m_localAngularSpeed.y;
    m_localAngularSpeed.z = relativePosition.x * impulse.y - impulse.x * relativePosition.y
                          + m_localAngularSpeed.z;
    snapSmall(m_localAngularSpeed);
}

// Accumulates mean linear and rotational friction for this step, using the moment
// about absolute position. Disabled nodes ignore it; parent-fixed nodes warn and apply nothing.
void PhysicsNode::addFrictionAtAbsolutePosition(const Vec3& absolutePosition, const Vec3& friction)
{
    if (!m_enabled)                                                                  // byte +0x0C
        return;

    IPhysicsNode* parent = getParent();                                              // vslot +0x34
    if (parent && m_fixToParent)                                                     // byte +0x11C
    {
        Debug(DEBUGTYPE_6, "Physics/PhysicsNode.cpp", 451) << "addFriction only works on rootParents.";
        return;
    }

    const float count = static_cast<float>(m_dwordF4);                               // +0xF4
    const float next = static_cast<float>(m_dwordF4 + 1);
    const float inv = 1.0f / next;
    Vec3 sum;
    sum.x = m_positionalFriction.x * count + friction.x;                             // +0xC4
    sum.y = m_positionalFriction.y * count + friction.y;
    sum.z = m_positionalFriction.z * count + friction.z;
    m_positionalFriction.x = sum.x * inv;
    m_positionalFriction.y = sum.y * inv;
    m_positionalFriction.z = inv * sum.z;

    const Vec3& origin = getAbsolutePosition();                                      // vslot +0x48
    const float rx = absolutePosition.x - origin.x;
    const float ry = absolutePosition.y - origin.y;
    const float rz = absolutePosition.z - origin.z;
    Vec3 moment;
    moment.x = ry * friction.z - rz * friction.y;
    moment.y = rz * friction.x - rx * friction.z;
    moment.z = rx * friction.y - friction.x * ry;
    Vec3 sum2;
    sum2.x = moment.x + m_rotationalFriction.x * count;                              // +0xD0
    sum2.y = moment.y + m_rotationalFriction.y * count;
    sum2.z = moment.z + m_rotationalFriction.z * count;
    const float inv2 = 1.0f / static_cast<float>(m_dwordF4 + 1);
    m_rotationalFriction.x = sum2.x * inv2;
    m_rotationalFriction.y = sum2.y * inv2;
    m_rotationalFriction.z = inv2 * sum2.z;
    ++m_dwordF4;
}

// Returns root linear velocity plus angular velocity crossed with the point's lever arm.
// The result references scratch storage overwritten by the next call; no root returns the shared zero vector.
const Vec3& PhysicsNode::getTangentSpeed(const Vec3& absolutePosition)
{
    IPhysicsNode* root = getRootNode();                                              // +0x150
    if (!root)
        return Vec3::getZero();

    const Vec3& rootPosition = root->getAbsolutePosition();                          // +0x48
    const float rx = absolutePosition.x - rootPosition.x;
    const float ry = absolutePosition.y - rootPosition.y;
    const float rz = absolutePosition.z - rootPosition.z;

    const Vec3& w = root->getRotationalSpeed();                                      // +0x64
    m_vec12C.x = w.y * rz - w.z * ry;                                                // +0x12C
    m_vec12C.y = w.z * rx - rz * w.x;
    m_vec12C.z = ry * w.x - rx * w.y;
    snapSmall(m_vec12C);

    const Vec3& v = root->getPositionalSpeed();                                      // +0x60
    m_vec12C.x += v.x;
    m_vec12C.y += v.y;
    m_vec12C.z += v.z;
    return m_vec12C;
}

// getTangentAcceleration - / Acceleration at a world
// point on the root rigid body:

//   root linear acceleration + alpha x radius + omega x (omega x radius)

// The two rotational contributions are epsilon-snapped separately before the unsnapped final sum.
// The result, like getTangentSpeed's, is a reference to this node's scratch member. Neither shipped
// port null-checks getRootNode before dereferencing it.
const Vec3& PhysicsNode::getTangentAcceleration(const Vec3& absolutePosition)
{
    IPhysicsNode* const root = getRootNode();
    const Vec3& rootPosition = root->getAbsolutePosition();
    const float rx = absolutePosition.x - rootPosition.x;
    const float ry = absolutePosition.y - rootPosition.y;
    const float rz = absolutePosition.z - rootPosition.z;

    const Vec3& angularAcceleration = root->getRotationalAcceleration();
    Vec3 tangential;
    tangential.x = angularAcceleration.y * rz - angularAcceleration.z * ry;
    tangential.y = angularAcceleration.z * rx - angularAcceleration.x * rz;
    tangential.z = angularAcceleration.x * ry - angularAcceleration.y * rx;
    snapSmall(tangential);

    const Vec3& angularSpeed = root->getRotationalSpeed();
    Vec3 angularSpeedCrossRadius;
    angularSpeedCrossRadius.x = angularSpeed.y * rz - angularSpeed.z * ry;
    angularSpeedCrossRadius.y = angularSpeed.z * rx - angularSpeed.x * rz;
    angularSpeedCrossRadius.z = angularSpeed.x * ry - angularSpeed.y * rx;

    Vec3 centripetal;
    centripetal.x = angularSpeed.y * angularSpeedCrossRadius.z
                  - angularSpeed.z * angularSpeedCrossRadius.y;
    centripetal.y = angularSpeed.z * angularSpeedCrossRadius.x
                  - angularSpeed.x * angularSpeedCrossRadius.z;
    centripetal.z = angularSpeed.x * angularSpeedCrossRadius.y
                  - angularSpeed.y * angularSpeedCrossRadius.x;
    snapSmall(centripetal);

    const Vec3& linear = root->getPositionalAcceleration();
    m_vec138.x = (centripetal.x + linear.x) + tangential.x;
    m_vec138.y = (centripetal.y + linear.y) + tangential.y;
    m_vec138.z = (centripetal.z + linear.z) + tangential.z;
    return m_vec138;
}

const Vec3& PhysicsNode::getRotationalAcceleration() const { return m_rotationalAcceleration; }
const Vec3& PhysicsNode::getPositionalFriction() const     { return m_positionalFriction; }
const Vec3& PhysicsNode::getRotationalFriction() const     { return m_rotationalFriction; }

void PhysicsNode::setDragOffset(const Vec3& v)             { m_dragOffset = v; }
const Vec3& PhysicsNode::getDragOffset() const             { return m_dragOffset; }
void PhysicsNode::setDragModifier(const Vec3& v)           { m_dragModifier = v; }
const Vec3& PhysicsNode::getDragModifier() const           { return m_dragModifier; }
void PhysicsNode::setCenterOfMassOffset(const Vec3& v)     { m_centerOfMassOffset = v; }
const Vec3& PhysicsNode::getCenterOfMassOffset() const     { return m_centerOfMassOffset; }
void PhysicsNode::setCenterOfCollisionOffset(const Vec3& v){ m_centerOfCollisionOffset = v; }
const Vec3& PhysicsNode::getCenterOfCollisionOffset() const{ return m_centerOfCollisionOffset; }
void PhysicsNode::setInertiaModifier(const Vec3& v)        { m_inertiaModifier = v; }
const Vec3& PhysicsNode::getInertiaModifier() const        { return m_inertiaModifier; }
void PhysicsNode::setGravityModifier(float v)              { m_gravityModifier = v; }

// see the header.
void PhysicsNode::reset(bool full)
{
    PointPhysicsNode::reset(full);
    zero(m_rotationalSpeed);
    zero(m_angularIntegrationVelocity);
    zero(m_rotationalAcceleration);
    zero(m_positionalFriction);
    zero(m_rotationalFriction);
    m_dwordF4 = 0;
    zero(m_localAngularSpeed);
    m_hasContact = false;
    m_isInAir = true;
    if (full)
    {
        m_prevTransformationIsObjects = true;
        m_latencyIndex = 0;
        if (m_latencyPositions)
        {
            for (int i = 0; i < 16; ++i)
            {
                m_latencyPositions[i] = Vec3::getZero();
                m_latencyRotations[i].w = 1.0f;
                m_latencyRotations[i].x = 0.0f;
                m_latencyRotations[i].y = 0.0f;
                m_latencyRotations[i].z = 0.0f;
            }
        }
    }
}

bool PhysicsNode::getIsPoint() const { return false; }

// the two rings - positions zeroed, rotations identity.
void PhysicsNode::initLatencyCompensation()
{
    Vec3* positions = new Vec3[16];
    for (int i = 0; i < 16; ++i)
        positions[i].x = positions[i].y = positions[i].z = 0.0f;
    m_latencyPositions = positions;
    Quat* rotations = new Quat[16];
    for (int i = 0; i < 16; ++i)
    {
        rotations[i].w = 1.0f;
        rotations[i].x = rotations[i].y = rotations[i].z = 0.0f;
    }
    m_latencyRotations = rotations;
}

bool PhysicsNode::hasLatencyCompensation() { return m_latencyPositions != 0; }

// Appends this frame's position and orientation to allocated latency rings; does nothing without them.
// Preserve the quaternion's initial component stores before fromMat.
void PhysicsNode::updateLatencyState()
{
    if (!m_latencyPositions)                                                         // +0x120
        return;

    const Mat4& transform = getAbsoluteTransformation();                             // vslot +0x3C

    Vec3& position = m_latencyPositions[m_latencyIndex];                             // +0x128
    position.x = transform.m[12];
    position.y = transform.m[13];
    position.z = transform.m[14];

    Quat rotation;
    rotation.x = 0.0f; rotation.y = 0.0f; rotation.z = 0.0f;                         // dead, as compiled
    rotation.fromMat(transform);
    m_latencyRotations[m_latencyIndex] = rotation;                                   // +0x124

    ++m_latencyIndex;
    if (m_latencyIndex == 16)
        m_latencyIndex = 0;
}

// getLatencyCompensatedTransformation - The vehicle/body twin of the
// SoldierPhysicsNode reader: 16 samples at 30 Hz, with the integral history distance capped at
// fourteen so both interpolation neighbours remain available. It differs only by having no
// enabled gate, no soldier pose output, and separate position/rotation arrays.

// The returned matrix is one function-static identity matrix, exactly as shipped. The optional
// int pointer is unused on this class; only SoldierPhysicsNode reports a historical pose through it.
const Mat4& PhysicsNode::getLatencyCompensatedTransformation(float latency, int*)
{
    const float samplePeriod = 0.033333335f;
    if (!m_latencyPositions || latency <= samplePeriod)
        return getAbsoluteTransformation();

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
    rotation.slerp(m_latencyRotations[olderIndex], m_latencyRotations[newerIndex], blend);

    static Mat4 compensated = Mat4::getIdentity();
    rotation.toMat(compensated);
    compensated.m[12] = m_latencyPositions[olderIndex].x
        + (m_latencyPositions[newerIndex].x - m_latencyPositions[olderIndex].x) * blend;
    compensated.m[13] = m_latencyPositions[olderIndex].y
        + (m_latencyPositions[newerIndex].y - m_latencyPositions[olderIndex].y) * blend;
    compensated.m[14] = m_latencyPositions[olderIndex].z
        + (m_latencyPositions[newerIndex].z - m_latencyPositions[olderIndex].z) * blend;
    return compensated;
}


IPhysicsNode* PhysicsNode::getRootNode()
{
    return static_cast<IPhysicsNode*>(L::physicsNode(getRootParent(m_object)));
}

const Vec3& PhysicsNode::getAngularIntegrationVelocity() const { return m_angularIntegrationVelocity; }
void PhysicsNode::setAngularIntegrationVelocity(const Vec3& v) { m_angularIntegrationVelocity = v; }
void PhysicsNode::setLocalAngularSpeed(const Vec3& v)          { m_localAngularSpeed = v; }
const Vec3& PhysicsNode::getLocalAngularSpeed() const          { return m_localAngularSpeed; }

void PhysicsNode::setHasContact(bool v)        { m_hasContact = v; }
bool PhysicsNode::getHasContact() const        { return m_hasContact; }
void PhysicsNode::setIsInAir(bool v)           { m_isInAir = v; }
bool PhysicsNode::getIsInAir() const           { return m_isInAir; }
void PhysicsNode::setIsPlatformPhysics(bool v) { m_isPlatformPhysics = v; }
bool PhysicsNode::getIsPlatformPhysics() const { return m_isPlatformPhysics; }

// the loop-invariant flag test is the ELF's, kept.
PhysicsNode* PhysicsNode::getTopFixParent()
{
    PhysicsNode* top = this;
    for (IPhysicsNode* p = getParent(); p && m_fixToParent; p = p->getParent())
        top = static_cast<PhysicsNode*>(p);
    return top;
}


float PhysicsNode::getDragMod(float u)
{
    return u * 25.0f + 1.0f - u;
}

// Physics integration tunables retain value/pointer pairs. Local names are [CHOSEN].
namespace {
    float g_posDampingValue        = 0.99f;  float* const g_posDamping        = &g_posDampingValue;        // ds:0x8ABA484 / 488
    float g_sleepSpeedValue        = 0.5f;   float* const g_sleepSpeed        = &g_sleepSpeedValue;        // ds:0x8ABA4AC / 4B0
    float g_sleepRotSpeedValue     = 0.5f;   float* const g_sleepRotSpeed     = &g_sleepRotSpeedValue;     // ds:0x8ABA4B4 / 4B8
    float g_localLinearYDampValue  = 0.5f;   float* const g_localLinearYDamp  = &g_localLinearYDampValue;  // ds:0x8ABA4C4 / 4C8
    float g_gravityCompSizeValue   = 0.5f;   float* const g_gravityCompSize   = &g_gravityCompSizeValue;   // ds:0x8ABA4CC / 4D0
    float g_inAirGravityModValue   = 0.1f;   float* const g_inAirGravityMod   = &g_inAirGravityModValue;   // ds:0x8ABA4D4 / 4D8
    float g_rotDampingValue        = 0.99f;  float* const g_rotDamping        = &g_rotDampingValue;        // ds:0x8ABA48C / 490
    float g_rotDampingWaterValue   = 0.9f;   float* const g_rotDampingWater   = &g_rotDampingWaterValue;   // ds:0x8ABA49C / 4A0
    float g_inertiaScaleValue      = 1.5f;   float* const g_inertiaScale      = &g_inertiaScaleValue;      // ds:0x8ABA4BC / 4C0
}

namespace {

// updateRotationalPhysics applies each of the three inertia axes separately, and to do that it
// projects the vector it is about to integrate onto the corresponding row of the object's
// transformation. Inlined six times in the original (three rows, twice over); one helper here.

// The guard is on fabs(lenSq) even though a squared length cannot be negative - as compiled.
Vec3 projectOntoAxis(const Vec3& v, const float* row)
{
    Vec3 out;
    const float lenSq = row[1] * row[1] + row[0] * row[0] + row[2] * row[2];
    if (fabsf(lenSq) > 1.19209290e-7f)                                               // FLT_EPSILON
    {
        const float k = (v.y * row[1] + v.x * row[0] + v.z * row[2]) / lenSq;
        out.x = row[0] * k;
        out.y = row[1] * k;
        out.z = row[2] * k;
    }
    else
    {
        out.x = 0.0f; out.y = 0.0f; out.z = 0.0f;
    }
    return out;
}

} // namespace

// Advances node sleep tracking and integration for dt; disabled nodes do nothing.
// Awake nonsoldier roots use motion histories for sleep, then apply drag, angular/linear integration
// and gravity. Children follow root sleepiness; sleeping contacts wake when their support moves.
void PhysicsNode::updatePhysics(float dt)
{
    if (!m_enabled)                                                                  // +0x0C
        return;
    IPhysicsNode* root = getRootNode();                                              // +0x150
    if (m_sleepiness >= 0 && root == this
        && L::objectTemplate(m_object)->getClassID() != CID_SoldierTemplate)         // object +0x14 -> +0x10
    {
        if (!m_byte144)
        {
            m_byte144 = true;                                                        // +0x144
            m_oldPosition = getAbsolutePosition();                                   // +0x148 <- +0x48
        }
        const Vec3 old = m_oldPosition;
        const Vec3& pos = getAbsolutePosition();
        m_oldPosition = pos;
        const float invDt = 1.0f / -dt;
        Vec3 v;
        v.x = (old.x - pos.x) * invDt;
        v.y = (old.y - pos.y) * invDt;
        v.z = (old.z - pos.z) * invDt;
        Vec3& slot3 = m_vec1AC[m_dword1D0];                                          // the 3-ring
        m_vec1A0.x -= slot3.x; m_vec1A0.y -= slot3.y; m_vec1A0.z -= slot3.z;         // the running sum
        slot3 = v;
        m_vec1A0.x += v.x; m_vec1A0.y += v.y; m_vec1A0.z += v.z;
        m_dword1D0 = (m_dword1D0 + 1) % 3;
        const float sum3 = m_vec1A0.x * m_vec1A0.x + m_vec1A0.y * m_vec1A0.y + m_vec1A0.z * m_vec1A0.z;
        bool awake = false;
        if (*g_sleepSpeed * *g_sleepSpeed * 3.0f * 3.0f <= sum3)
            awake = true;
        else
        {
            Vec3& slot5 = m_vec160[m_dword19C];                                      // the 5-ring
            m_vec154.x -= slot5.x; m_vec154.y -= slot5.y; m_vec154.z -= slot5.z;
            slot5 = m_rotationalSpeed;                                               // +0x94
            m_vec154.x += m_rotationalSpeed.x; m_vec154.y += m_rotationalSpeed.y; m_vec154.z += m_rotationalSpeed.z;
            m_dword19C = (m_dword19C + 1) % 5;
            const float sum5 = m_vec154.x * m_vec154.x + m_vec154.y * m_vec154.y + m_vec154.z * m_vec154.z;
            awake = (*g_sleepRotSpeed * *g_sleepRotSpeed * 5.0f * 5.0f <= sum5);
        }
        if (awake) setIsAwake();                                                     // +0x130
        else       setIsSleepier();                                                  // +0x12C
    }
    if (const IObjectTemplate* t = L::objectTemplate(getObject()))                   // +0x2C -> +0x14
        const_cast<IObjectTemplate*>(t)->queryImplementation(CID_SimpleObjectTemplate);   // +0x50 - the answer is dropped, as shipped
    if (!getIsSleeping() && root == this)                                            // +0x11C
    {
        if (m_drag > 0.0f)                                                           // +0x50
        {
            if (L::flags(m_object) & 0x02000000u)                                    // object +0x07 bit 1
            {
                updatePositionalDragSimple(0.1f, 1.0f);
            }
            else if (ICollisionMesh* mesh = static_cast<ICollisionMesh*>(L::collisionMesh(m_object)))   // +0x40
            {
                if (mesh->hasLod(1))                                                 // slot 11
                {
                    const LocalBoundingBox* box = static_cast<const LocalBoundingBox*>(mesh->getBoundingBox(1));   // slot 22
                    const float height = box->m_max.y - box->m_min.y;
                    const float depth  = box->m_max.z - box->m_min.z;
                    const float width  = box->m_max.x - box->m_min.x;
                    float water = m_underWater / height;                             // +0x5C
                    if (water > 1.0f)
                        water = 1.0f;
                    const float wpi = width * 3.1400001f;
                    const float ax = height * 3.1400001f * 0.25f * depth * m_dragModifier.x;   // +0xE8
                    const float ay = depth * (0.25f * wpi) * m_dragModifier.y;
                    const float az = height * (0.25f * wpi) * m_dragModifier.z;
                    updatePositionalDragAdvanced(az, ay, ax, getDragMod(water));
                    updateRotationalDragAdvanced(az, ay, ax, getDragMod(water));
                }
            }
        }
        updateRotationalPhysics(dt);
        updatePositionalPhysics(dt);
        float g = 1.0f;
        if (getShouldGravityCompensateOnContact() && m_hasContact && getPositionalSpeed().y < 0.0f)   // +0x1A0, +0x228, +0x60
            g = *g_inAirGravityMod;
        m_hasContact = false;                                                        // +0x228
        g *= m_gravityModifier;                                                      // +0x58
        m_positionalAcceleration.y += static_cast<IPhysicsSystem*>(basicPhysicsSystem)->getGravity() * g;   // +0x30 <- +0x20
        m_isInAir = true;                                                            // +0x229
        return;
    }
    if (root)
        setSleepiness(root->getSleepiness());                                        // +0x124 <- +0x128
    if (getIsSleeping())
        reset(false);                                                                // +0x118
    IResponsePhysics* rp = static_cast<IResponsePhysics*>(L::responsePhysics(getObject()));   // object +0x48
    if (rp->getClassID() == CID_ResponsePhysics)                                     // +0x24
    {
        ResponsePhysics* r = static_cast<ResponsePhysics*>(rp);
        if (int* block = r->m_sharedF4)                                              // +0xF4 - a WeakPtr<IObject> control block
        {
            // The block contains a native pointer after its int reference count: +4 on x86,
            // +8 on x64. This is an address, never a 32-bit object/network identifier.
            IObject* restingOn = reinterpret_cast<WeakPtr<IObject>::Ctrl*>(block)->target;
            IPhysicsNode* node = restingOn ? static_cast<IPhysicsNode*>(L::physicsNode(restingOn)) : 0;   // +0x44
            if (!restingOn || !node->getIsSleeping())
            {
                setIsAwake();
                if (--block[0] == 0)
                    ::operator delete(block);
                r->m_sharedF4 = 0;
            }
        }
    }
}

// Integrates enabled, non-platform motion with acceleration capped at 1000 and excessive
// friction discarded. Folds pending impulses into damped speed, advances position with
// trapezoidal velocity, then clears acceleration, friction, and pending local impulses.
void PhysicsNode::updatePositionalPhysics(float dt)
{
    if (!m_enabled || m_isPlatformPhysics)
        return;
    const float a2 = m_positionalAcceleration.x * m_positionalAcceleration.x
                   + m_positionalAcceleration.y * m_positionalAcceleration.y
                   + m_positionalAcceleration.z * m_positionalAcceleration.z;
    if (a2 > 1000000.0f)
    {
        const float s = 1000.0f / sqrtf(a2);
        m_positionalAcceleration.x *= s; m_positionalAcceleration.y *= s; m_positionalAcceleration.z *= s;
    }
    if (m_positionalFriction.x * m_positionalFriction.x + m_positionalFriction.y * m_positionalFriction.y
        + m_positionalFriction.z * m_positionalFriction.z > 62500.0f)                 // +0xC4
    {
        m_positionalFriction.x = m_positionalFriction.y = m_positionalFriction.z = 0.0f;
    }
    m_localLinearSpeed.y *= *g_localLinearYDamp;                                    // +0x3C
    addPositionalSpeed(m_localLinearSpeed);                                          // +0x70 with +0x38
    const Vec3 oldSpeed = m_positionalSpeed;                                         // +0x20
    Vec3 add;
    add.x = (m_positionalFriction.x + m_positionalAcceleration.x) * dt;
    add.y = (m_positionalFriction.y + m_positionalAcceleration.y) * dt;
    add.z = (m_positionalFriction.z + m_positionalAcceleration.z) * dt;
    addPositionalSpeed(add);
    if (m_underWater > 0.050000001f)
        L::objectTemplate(m_object)->getClassID();                                   // dropped, as shipped
    m_positionalSpeed.x *= *g_posDamping; m_positionalSpeed.y *= *g_posDamping; m_positionalSpeed.z *= *g_posDamping;
    if (fabsf(m_positionalSpeed.x) < 0.000001f) m_positionalSpeed.x = 0.0f;
    if (fabsf(m_positionalSpeed.y) < 0.000001f) m_positionalSpeed.y = 0.0f;
    if (fabsf(m_positionalSpeed.z) < 0.000001f) m_positionalSpeed.z = 0.0f;
    m_linearIntegrationVelocity.x = (m_positionalSpeed.x + oldSpeed.x) * 0.5f;      // +0x44
    m_linearIntegrationVelocity.y = (m_positionalSpeed.y + oldSpeed.y) * 0.5f;
    m_linearIntegrationVelocity.z = 0.5f * (m_positionalSpeed.z + oldSpeed.z);
    if (fabsf(m_linearIntegrationVelocity.x) < 0.000001f) m_linearIntegrationVelocity.x = 0.0f;
    if (fabsf(m_linearIntegrationVelocity.y) < 0.000001f) m_linearIntegrationVelocity.y = 0.0f;
    if (fabsf(m_linearIntegrationVelocity.z) < 0.000001f) m_linearIntegrationVelocity.z = 0.0f;
    const Vec3& pos = getAbsolutePosition();                                         // +0x48
    Vec3 next;
    next.x = pos.x + dt * m_linearIntegrationVelocity.x;
    next.y = pos.y + dt * m_linearIntegrationVelocity.y;
    next.z = pos.z + dt * m_linearIntegrationVelocity.z;
    setAbsolutePosition(next);                                                       // +0x4C
    m_positionalAcceleration.x = m_positionalAcceleration.y = m_positionalAcceleration.z = 0.0f;
    m_positionalFriction.x = m_positionalFriction.y = m_positionalFriction.z = 0.0f;
    m_localLinearSpeed.x = m_localLinearSpeed.y = m_localLinearSpeed.z = 0.0f;       // +0x38
    m_localAngularSpeed.x = m_localAngularSpeed.y = m_localAngularSpeed.z = 0.0f;    // +0xAC
}

// Subtracts simple wind-relative drag from acceleration using radius, factor, drag,
// and mass. Disabled and platform nodes do nothing.
void PhysicsNode::updatePositionalDragSimple(float radius, float factor)
{
    if (!m_enabled || m_isPlatformPhysics)
        return;
    const Vec3 wind = static_cast<IPhysicsSystem*>(basicPhysicsSystem)->getWind();  // +0x28
    const float rx = m_positionalSpeed.x * factor - wind.x;
    const float ry = m_positionalSpeed.y * factor - wind.y;
    const float rz = factor * m_positionalSpeed.z - wind.z;
    const float k = 3.1415927f * radius * radius * m_drag;
    const float invMass = 1.0f / m_mass;
    m_positionalAcceleration.x -= rx * k * invMass;
    m_positionalAcceleration.y -= ry * k * invMass;
    m_positionalAcceleration.z -= rz * k * invMass;
}

// Caches whether contact should compensate gravity: only root, non-player-control
// objects with LOD 1 bounds below 0.5 on every axis qualify.
bool PhysicsNode::getShouldGravityCompensateOnContact() const
{
    if (m_gravityCompensateOnContact < 0)
    {
        PhysicsNode* self = const_cast<PhysicsNode*>(this);
        self->m_gravityCompensateOnContact = 0;
        IObject* object = self->getObject();                                         // +0x2C
        IObject* root = getRootParent(object);
        if (root == object && !root->queryInterface(IID_IPlayerControlObject))
        {
            if (!object->hasCollisionLod(1))                                         // slot 51
                return false;
            LocalBoundingBox* box = const_cast<LocalBoundingBox*>(static_cast<const LocalBoundingBox*>(
                static_cast<ICollisionMesh*>(L::collisionMesh(object))->getBoundingBox(1)));   // +0x40 -> slot 22
            if (box->m_derivedDataDirty)                                             // +0x20
                box->calcRadius();
            const float cap = *g_gravityCompSize;
            if (!(cap <= box->m_size.x) && !(cap <= box->m_size.y) && !(cap <= box->m_size.z))   // +0x24
                self->m_gravityCompensateOnContact = 1;
        }
    }
    return m_gravityCompensateOnContact > 0;
}


// Lazily caches inverse inertia from the LOD-1 box. Missing meshes return zero without latching.
// An existing mesh without LOD 1 aborts, despite the diagnostic's misleading no-mesh wording.
const Vec3& PhysicsNode::getInverseInertia()
{
    if (m_inverseInertiaValid)                                                       // byte +0x216
        return m_inverseInertia;                                                     // +0x218

    ICollisionMesh* mesh = static_cast<ICollisionMesh*>(L::collisionMesh(m_object));  // object +0x40
    if (mesh)
    {
        if (!m_object->hasCollisionLod(1))                                           // slot 51
        {
            Debug(DEBUGTYPE_6, "Physics/PhysicsNode.cpp", 970, "")
                << "When calculation inertia there is no collision mesh for object "
                << L::objectTemplate(getObject())->getName();                        // +0x2C -> +0x14 -> slot 11
        }

        // const_cast: the extents are read through the box's own recompute-if-dirty accessor, and
        // that recompute writes back into the mesh's box. The original does the same - it calls
        // LocalBoundingBox::calcRadius on what slot 22 handed it.
        LocalBoundingBox* box = const_cast<LocalBoundingBox*>(
            static_cast<const LocalBoundingBox*>(mesh->getBoundingBox(1)));          // slot 22
        const float sx = box->getSize().x;
        const float sy = box->getSize().y;
        const float sz = box->getSize().z;
        const float zz = sz * sz;
        const float xx = sx * sx;

        m_inverseInertia.x = 3.0f / (sy * sy + zz);
        m_inverseInertia.y = 3.0f / (zz + xx);
        m_inverseInertia.z = 3.0f / (sy * sy + xx);
        m_inverseInertiaValid = true;
    }
    return m_inverseInertia;
}

// Adds simple rotational drag acceleration using radius, factor, mass, and angular speed.
// The regular physics tick currently selects other drag paths.
void PhysicsNode::updateRotationalDragSimple(float radius, float factor)
{
    if (!m_enabled || m_isPlatformPhysics)
        return;

    const float k = radius * radius * -3.1415927f * m_drag * factor / m_mass;        // +0x50, +0x54
    m_rotationalAcceleration.x += m_rotationalSpeed.x * k;                           // +0xA0 <- +0x94
    m_rotationalAcceleration.y += m_rotationalSpeed.y * k;
    m_rotationalAcceleration.z += m_rotationalSpeed.z * k;
    snapSmall(m_rotationalAcceleration);
}

// updatePositionalDragAdvanced - / The velocity relative
// to wind is split across all three (possibly scaled) object axes. Each projection is weighted by
// its supplied cross-sectional area, recombined in world space and limited to one 1/30-second step
// before being applied at the rotated centre of the lod-1 collision box. Without lod 1 the force is
// applied at the origin. Both ports perform the same operations in this order.
void PhysicsNode::updatePositionalDragAdvanced(float areaZ, float areaY, float areaX,
                                                float velocityFactor)
{
    if (!m_enabled || m_isPlatformPhysics)
        return;

    const Mat4& transformation = getAbsoluteTransformation();
    const Vec3 wind = static_cast<IPhysicsSystem*>(basicPhysicsSystem)->getWind();
    const Vec3 relativeVelocity = {
        m_positionalSpeed.x * velocityFactor - wind.x,
        m_positionalSpeed.y * velocityFactor - wind.y,
        m_positionalSpeed.z * velocityFactor - wind.z
    };
    const float dragScale = sqrtf(relativeVelocity.x * relativeVelocity.x
                                + relativeVelocity.y * relativeVelocity.y
                                + relativeVelocity.z * relativeVelocity.z)
                          * m_drag / m_mass;

    const Vec3 axisX = { transformation.m[0], transformation.m[1], transformation.m[2] };
    const Vec3 axisY = { transformation.m[4], transformation.m[5], transformation.m[6] };
    const Vec3 axisZ = { transformation.m[8], transformation.m[9], transformation.m[10] };
    const Vec3 projectedX = projectOntoAxis(relativeVelocity, axisX);
    const Vec3 projectedY = projectOntoAxis(relativeVelocity, axisY);
    const Vec3 projectedZ = projectOntoAxis(relativeVelocity, axisZ);

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
    drag.x = clipDragComponent(m_positionalSpeed.x, drag.x) * 30.0f;
    drag.y = clipDragComponent(m_positionalSpeed.y, drag.y) * 30.0f;
    drag.z = clipDragComponent(m_positionalSpeed.z, drag.z) * 30.0f;

    Vec3 applicationPosition = Vec3::getZero();
    if (m_object->hasCollisionLod(1))
    {
        ICollisionMesh* const collisionMesh = static_cast<ICollisionMesh*>(L::collisionMesh(m_object));
        const LocalBoundingBox* const box = collisionMesh->getBoundingBox(1);
        const Vec3 localCentre = {
            (box->m_min.x + box->m_max.x) * 0.5f,
            0.0f,
            (box->m_min.z + box->m_max.z) * 0.5f
        };
        applicationPosition = rotateVector(transformation, localCentre);
    }

    const Vec3 acceleration = { -drag.x, -drag.y, -drag.z };
    addAccelerationAtRelativePosition(applicationPosition, acceleration);
}

// updateRotationalDragAdvanced - / The angular-speed
// twin of the body above. Both ports agree on the unusual third-axis coefficient below: Z uses
// (areaZ + areaX), exactly like Y, rather than the physically tempting (areaY + areaX). This is a
// shipped quirk and is preserved verbatim.
void PhysicsNode::updateRotationalDragAdvanced(float areaZ, float areaY, float areaX,
                                                float velocityFactor)
{
    if (!m_enabled || m_isPlatformPhysics)
        return;

    const Mat4& transformation = getAbsoluteTransformation();
    const Vec3 angularSpeed = m_rotationalSpeed;
    const float dragScale = sqrtf(angularSpeed.x * angularSpeed.x
                                + angularSpeed.y * angularSpeed.y
                                + angularSpeed.z * angularSpeed.z)
                          * m_drag / m_mass * velocityFactor;

    const Vec3 axisX = { transformation.m[0], transformation.m[1], transformation.m[2] };
    const Vec3 axisY = { transformation.m[4], transformation.m[5], transformation.m[6] };
    const Vec3 axisZ = { transformation.m[8], transformation.m[9], transformation.m[10] };
    const Vec3 projectedX = projectOntoAxis(angularSpeed, axisX);
    const Vec3 projectedY = projectOntoAxis(angularSpeed, axisY);
    const Vec3 projectedZ = projectOntoAxis(angularSpeed, axisZ);

    const float xScale = dragScale * (areaZ + areaY);
    const float yzScale = dragScale * (areaZ + areaX);            // shipped: also used for Z
    Vec3 drag = {
        projectedX.x * xScale + projectedY.x * yzScale + projectedZ.x * yzScale,
        projectedX.y * xScale + projectedY.y * yzScale + projectedZ.y * yzScale,
        projectedX.z * xScale + projectedY.z * yzScale + projectedZ.z * yzScale
    };
    drag.x *= 0.033333335f;
    drag.y *= 0.033333335f;
    drag.z *= 0.033333335f;
    drag.x = clipDragComponent(m_rotationalSpeed.x, drag.x) * 30.0f;
    drag.y = clipDragComponent(m_rotationalSpeed.y, drag.y) * 30.0f;
    drag.z = clipDragComponent(m_rotationalSpeed.z, drag.z) * 30.0f;

    m_rotationalAcceleration.x -= drag.x;
    m_rotationalAcceleration.y -= drag.y;
    m_rotationalAcceleration.z -= drag.z;
    snapSmall(m_rotationalAcceleration);
}

// Integrates angular velocity and updates the object's absolute transform.
// Preserve acceleration clamping, excessive-friction rejection, temporary inertia scaling and
// water-dependent damping. The integration velocity is the trapezoidal average.
// Rotation mutates the matrix returned by the const-reference getter before calling its setter;
// nonzero angular velocity still invokes the setter when the composed angle is below epsilon.
// Clears angular acceleration, friction and local input after the step.
void PhysicsNode::updateRotationalPhysics(float dt)
{
    if (!m_enabled || m_isPlatformPhysics)                                           // +0x0C, +0x215
        return;

    // 1. the two caps.
    const float accSq = m_rotationalAcceleration.y * m_rotationalAcceleration.y
                      + m_rotationalAcceleration.x * m_rotationalAcceleration.x
                      + m_rotationalAcceleration.z * m_rotationalAcceleration.z;
    if (accSq > 1000000.0f)
    {
        const float s = 1000.0f / sqrtf(accSq);
        m_rotationalAcceleration.x *= s;
        m_rotationalAcceleration.y *= s;
        m_rotationalAcceleration.z *= s;
    }

    const Vec3& invInertia = getInverseInertia();                                    // +0x15C
    const float* const m = getAbsoluteTransformation().m;                            // +0x3C

    if (m_rotationalFriction.x * m_rotationalFriction.x
      + m_rotationalFriction.y * m_rotationalFriction.y
      + m_rotationalFriction.z * m_rotationalFriction.z > 40000.0f)                  // +0xD0
    {
        m_rotationalFriction.x = 0.0f;
        m_rotationalFriction.y = 0.0f;
        m_rotationalFriction.z = 0.0f;
    }

    // 2. the inertia scale, in.
    m_inertiaModifier.x *= *g_inertiaScale;                                          // +0x110
    m_inertiaModifier.y *= *g_inertiaScale;
    m_inertiaModifier.z *= *g_inertiaScale;

    // 3. the local angular speed onto the three body axes.
    const Vec3 pz = projectOntoAxis(m_localAngularSpeed, m + 8);                     // +0xAC, row 2
    const float kz = invInertia.z / m_inertiaModifier.z;
    Vec3 delta;
    delta.x = pz.x * kz; delta.y = pz.y * kz; delta.z = pz.z * kz;

    const Vec3 py = projectOntoAxis(m_localAngularSpeed, m + 4);                     // row 1
    const float ky = invInertia.y / m_inertiaModifier.y;
    delta.x += py.x * ky; delta.y += py.y * ky; delta.z += py.z * ky;

    const Vec3 px = projectOntoAxis(m_localAngularSpeed, m);                         // row 0
    const float kx = invInertia.x / m_inertiaModifier.x;
    delta.x += px.x * kx; delta.y += px.y * kx; delta.z += px.z * kx;

    addRotationalSpeed(delta);                                                       // +0x74

    // 4. friction + acceleration, the same three projections, times dt.
    Vec3 force;
    force.x = m_rotationalAcceleration.x + m_rotationalFriction.x;
    force.y = m_rotationalFriction.y + m_rotationalAcceleration.y;
    force.z = m_rotationalFriction.z + m_rotationalAcceleration.z;

    const Vec3 fz = projectOntoAxis(force, m + 8);
    const float dz = dt * invInertia.z / m_inertiaModifier.z;
    Vec3 delta2;
    delta2.x = fz.x * dz; delta2.y = fz.y * dz; delta2.z = fz.z * dz;

    const Vec3 fy = projectOntoAxis(force, m + 4);
    const float dy = dt * invInertia.y / m_inertiaModifier.y;
    delta2.x += fy.x * dy; delta2.y += fy.y * dy; delta2.z += fy.z * dy;

    const Vec3 fx = projectOntoAxis(force, m);
    const float dx = dt * invInertia.x / m_inertiaModifier.x;
    delta2.x += fx.x * dx; delta2.y += fx.y * dx; delta2.z += fx.z * dx;

    // the inertia scale, back out.
    const float unscale = 1.0f / *g_inertiaScale;
    m_inertiaModifier.x *= unscale;
    m_inertiaModifier.y *= unscale;
    m_inertiaModifier.z *= unscale;

    const Vec3 previous = m_rotationalSpeed;                                         // +0x94, before the add
    addRotationalSpeed(delta2);

    // 5. the damping.
    float damping = *g_rotDamping;                                                   // 0.99
    if (m_underWater > 0.050000001f)                                                 // +0x5C
    {
        bool heavy = false;
        const IObjectTemplate* t = L::objectTemplate(m_object);                      // object +0x14
        const unsigned int cid = t->getClassID();                                    // slot 4
        if (cid == CID_SimpleObjectTemplate)
            heavy = static_cast<const SimpleObjectTemplate*>(t)->m_floaterMod > 0.0099999998f;   // +0x88
        else if (cid == CID_ParticleTemplate)
            heavy = static_cast<const ParticleTemplate*>(t)->m_floaterMod > 0.0099999998f;       // +0xA0
        if (heavy)
            damping = *g_rotDampingWater;                                            // 0.9
    }
    m_rotationalSpeed.x *= damping;
    m_rotationalSpeed.y *= damping;
    m_rotationalSpeed.z *= damping;

    // 6. the trapezoidal average - this is what turns the body.
    m_angularIntegrationVelocity.x = (m_rotationalSpeed.x + previous.x) * 0.5f;      // +0xB8
    m_angularIntegrationVelocity.y = (m_rotationalSpeed.y + previous.y) * 0.5f;
    m_angularIntegrationVelocity.z = 0.5f * (m_rotationalSpeed.z + previous.z);
    snapSmall(m_angularIntegrationVelocity);

    // 7. the turn.
    const float wSq = m_angularIntegrationVelocity.y * m_angularIntegrationVelocity.y
                    + m_angularIntegrationVelocity.x * m_angularIntegrationVelocity.x
                    + m_angularIntegrationVelocity.z * m_angularIntegrationVelocity.z;
    if (wSq > 0.0000010000001f)
    {
        const float w = sqrtf(wSq);
        const float inv = 1.0f / w;
        Vec3 axis;
        axis.x = m_angularIntegrationVelocity.x * inv;
        axis.y = m_angularIntegrationVelocity.y * inv;
        axis.z = inv * m_angularIntegrationVelocity.z;

        // the object's own matrix, turned in place - see the header block.
        Mat4& transform = const_cast<Mat4&>(getAbsoluteTransformation());
        const float degrees = w * dt * 57.295776f;
        if (fabsf(degrees) > 1.19209290e-7f)
        {
            Mat4 rotation = Mat4::getIdentity();
            setRotateAboutLine(rotation, degrees, axis);
            const Mat4 before = transform;
            transform.rotMult(before, rotation);
        }
        setAbsoluteTransformation(transform);                                        // +0x40
    }

    // 8. cleared for the next tick.
    m_rotationalAcceleration.x = 0.0f; m_rotationalAcceleration.y = 0.0f; m_rotationalAcceleration.z = 0.0f;
    m_rotationalFriction.x = 0.0f;     m_rotationalFriction.y = 0.0f;     m_rotationalFriction.z = 0.0f;
    m_dwordF4 = 0;                                                                   // +0xF4
}

} // namespace world
} // namespace hfe
} // namespace dice
