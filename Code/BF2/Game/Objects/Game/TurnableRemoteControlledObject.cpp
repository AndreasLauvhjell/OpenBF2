// Turnable remote-controlled object and artillery mission behavior.

#include "TurnableRemoteControlledObject.h"

#include "../../../Core/Ids.h"
#include "../RemoteControlledObjectTemplate.h"
#include "../RotationalBundleTemplate.h"
#include "RotationalBundle.h"

#include <cfloat>
#include <cmath>

namespace dice {
namespace hfe {
namespace world {

namespace {

float& component(Vec3& value, int axis)
{
    return (&value.x)[axis];
}

const float& component(const Vec3& value, int axis)
{
    return (&value.x)[axis];
}

float projectedAngle(const Vec3& a, const Vec3& b, int firstAxis, int secondAxis)
{
    const float a0 = component(a, firstAxis);
    const float a1 = component(a, secondAxis);
    const float b0 = component(b, firstAxis);
    const float b1 = component(b, secondAxis);
    const float denominator = std::sqrt(a0 * a0 + a1 * a1) *
                              std::sqrt(b0 * b0 + b1 * b1);
    float angle = 0.0f;
    if (denominator != 0.0f)
    {
        float cosine = (a0 * b0 + a1 * b1) / denominator;
        if (cosine <= 1.0f)
        {
            if (cosine < -1.0f)
                cosine = -1.0f;
            angle = std::acos(cosine);
        }
    }
    return angle;
}

} // namespace

// RemoteControlledObject(template); the vptrs; the own block (count 1, the
// template, -1.0 at +0x184, zeros; +0x170..+0x180 untouched).
TurnableRemoteControlledObject::TurnableRemoteControlledObject(const TurnableRemoteControlledObjectTemplate* objectTemplate)
    : RemoteControlledObject(objectTemplate)
    , m_refCount(1)                                                                  // +0x150
    , m_template(objectTemplate)                                                     // +0x154
    , m_targetDirection(Vec3::getZero())                                             // +0x158
    , m_initialHostForward(Vec3::getZero())                                          // +0x164
    , m_previousAngle(-1.0f)                                                         // +0x184
    , m_lastRotation(Vec3::getZero())                                                // +0x188
    , m_status(0)                                                                    // +0x194
{
}

TurnableRemoteControlledObject::~TurnableRemoteControlledObject() {}                 // ~RemoteControlledObject

// The quartet on the +0x150 count - 0x81CC620 / 0x81CC650 / 0x81CC670 / 0x81CC6C0.
void TurnableRemoteControlledObject::addRef()  { ++m_refCount; }
int  TurnableRemoteControlledObject::getRef()  { return m_refCount; }
int  TurnableRemoteControlledObject::release()
{
    const int left = m_refCount - 1;
    const bool last = (m_refCount == 1);
    m_refCount = left;
    if (last)
        delete this;                                                                 // the D0 slot
    return left;
}

void* TurnableRemoteControlledObject::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IObject)
        return const_cast<TurnableRemoteControlledObject*>(this);
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(const_cast<TurnableRemoteControlledObject*>(this));   // this + 0x80
    return 0;
}

// . The requested world direction is first expressed in the host parent's
// coordinate system. The two supported RC types then select the projection plane and turn sign.
void TurnableRemoteControlledObject::initMission(Vec3& target)
{
    if (!m_owner)
        return;

    IObject* parent = m_owner->getParent();
    if (parent)
    {
        Mat4 inverse;
        inverse.setInverse(parent->getAbsoluteTransformation());
        Vec3 local;
        local.x = inverse.m[0] * target.x + inverse.m[4] * target.y + inverse.m[8]  * target.z;
        local.y = inverse.m[1] * target.x + inverse.m[5] * target.y + inverse.m[9]  * target.z;
        local.z = inverse.m[2] * target.x + inverse.m[6] * target.y + inverse.m[10] * target.z;
        target = local;
    }
    m_targetDirection = target;

    const Mat4& hostTransform = m_owner->getRelativeTransformation();
    m_initialHostForward.x = hostTransform.m[8];
    m_initialHostForward.y = hostTransform.m[9];
    m_initialHostForward.z = hostTransform.m[10];

    if (m_template->rcType == 4)
    {
        m_projectionAxis = 1;
        m_rotationAxis = 0;
        m_fixedAxis = 2;
    }
    else if (m_template->rcType == 5)
    {
        m_projectionAxis = 0;
        m_rotationAxis = 1;
        m_fixedAxis = 2;
    }

    const float side =
        -component(m_targetDirection, m_fixedAxis) * component(m_initialHostForward, m_rotationAxis) +
         component(m_targetDirection, m_rotationAxis) * component(m_initialHostForward, m_fixedAxis);
    m_turnDirection = side > 0.0f ? 1 : -1;
    if (m_template->rcType == 5)
        m_turnDirection = side > 0.0f ? -1 : 1;

    m_targetAngle = std::fabs(projectedAngle(m_targetDirection, m_initialHostForward,
                                             m_rotationAxis, m_fixedAxis));
    if (m_template->rcType == 5)
        m_targetAngle = -m_targetAngle;

    m_status = 1;
    m_previousAngle = -1.0f;
    m_lastRotation = Vec3::getZero();
}

// . Status 1 is turning, 2 reached the angle and 3 means the movement stalled
// (or the host disappeared). The comparison against the previous angle is intentionally strict.
int TurnableRemoteControlledObject::updateMission(float deltaTime)
{
    if (!m_owner)
        return 3;
    if (m_status != 1)
        return m_status;

    const float turnRadians = deltaTime * m_template->turnVelocity;
    const Mat4& hostTransform = m_owner->getRelativeTransformation();
    Vec3 currentForward;
    currentForward.x = hostTransform.m[8];
    currentForward.y = hostTransform.m[9];
    currentForward.z = hostTransform.m[10];

    float angle = projectedAngle(m_initialHostForward, currentForward,
                                 m_rotationAxis, m_fixedAxis);
    const float pi = 3.1415927f;
    if (angle > pi)
        angle -= pi;
    if (angle < -pi)
        angle += pi;
    angle = std::fabs(angle);

    if (m_previousAngle < angle)
    {
        if (std::fabs(m_targetAngle) > angle)
        {
            m_previousAngle = angle;

            Vec3 step = Vec3::getZero();
            component(step, m_rotationAxis) =
                static_cast<float>(m_turnDirection) * turnRadians * 57.295776f;

            Mat4 rotation = Mat4::getIdentity();
            setRotationZXY(rotation, -step.z, -step.y, -step.x);
            Mat4 result;
            result.mult(rotation, hostTransform);
            setHostObjectRelativeTransformation(result);
            return m_status;
        }

        m_status = 2;
        return m_status;
    }

    m_status = 3;
    return m_status;
}

// . RotationalBundle hosts receive Euler state and applyRotation; other hosts get
// the matrix directly. A zero rotation limit is the shipped "unbounded" sentinel.
void TurnableRemoteControlledObject::setHostObjectRelativeTransformation(Mat4& transform)
{
    if (!m_owner)
        return;

    const IObjectTemplate* objectTemplate = m_owner->getTemplate();
    RotationalBundleTemplate* rotationalTemplate = objectTemplate
        ? static_cast<RotationalBundleTemplate*>(
              const_cast<IObjectTemplate*>(objectTemplate)->queryImplementation(
                  CID_RotationalBundleTemplate))
        : 0;

    if (!rotationalTemplate)
    {
        m_owner->setRelativeTransformation(transform);
        return;
    }

    float z;
    float x;
    float y;
    getRotationZXY(transform, z, x, y);
    Vec3 rotation;
    rotation.x = -y;
    rotation.y = -x;
    rotation.z = -z;

    for (int axis = 0; axis < 3; ++axis)
    {
        const float minimum = component(rotationalTemplate->m_minRotation, axis);
        const float maximum = component(rotationalTemplate->m_maxRotation, axis);
        const float value = component(rotation, axis);
        if ((std::fabs(minimum) > FLT_EPSILON && value < minimum) ||
            (std::fabs(maximum) > FLT_EPSILON && value > maximum))
            return;
    }

    RotationalBundle* host = static_cast<RotationalBundle*>(m_owner);
    host->setCurrentRotation(rotation);

    if (std::fabs(m_lastRotation.x) > FLT_EPSILON ||
        std::fabs(m_lastRotation.y) > FLT_EPSILON ||
        std::fabs(m_lastRotation.z) > FLT_EPSILON)
    {
        Vec3 speed;
        speed.x = (rotation.x - m_lastRotation.x) * 0.033333335f;
        speed.y = (rotation.y - m_lastRotation.y) * 0.033333335f;
        speed.z = (rotation.z - m_lastRotation.z) * 0.033333335f;
        host->setCurrentRotationalSpeed(speed);
        m_lastRotation = rotation;
    }

    host->applyRotation(rotation);
}

int TurnableRemoteControlledObject::getStatus() const { return m_status; }

} // namespace world
} // namespace hfe
} // namespace dice
