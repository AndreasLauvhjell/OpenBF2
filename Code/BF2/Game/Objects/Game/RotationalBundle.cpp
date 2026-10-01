// Rotational-bundle input, motion, and lifecycle.

#include "RotationalBundle.h"
#include "../GameLogicComp/IArmorGLComp.h"

#include <cfloat>                                  // FLT_EPSILON - calculateAndClipAngle
#include <cmath>
#include <string>

#include "../../../Core/Ids.h"
#include "../../../Core/Debug.h"                    // the attested RotationalBundle.cpp:540 line
#include "../../../Game/Utility/Vars.h"              // the sound-smooth-time var (updateRotationalSound)
#include "../../../Math/Math.h"                      // smooth_in_out
#include "../IObjectComponent.h"                     // the armor component word (updateRotationalSound)
#include "../ObjectLayoutAccess.h"                   // +0x3C armorComponent, +0x44 physicsNode
#include "../../../Physics/IPhysicsNode.h"           // getRotationalSpeed - updateRegulate
#include "../../../Scene/Object/ObjectUtil.h"        // setNetworkableComp
#include "../../../Sound/NullSound.h"                // ISound - the RotationRpm sound's surface
#include "../../../Sound/SoundObjectTemplate.h"      // initSoundTemplates' addSoundTemplate product
#include "../RotationalBundleTemplate.h"
#include "../../../Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"   // the PCO walk

namespace dice {
namespace hfe {
namespace world {

// The constructor - 0x81E9C50 (C2) / 0x81E9EA0 (C1), 580 B. Bundle(template); the three vptrs;
// the whole own block zeroed except +0x178 (1.0), +0x188 (1) and +0x198 (0.5); +0x196 set when the
// template's continousRotationSpeed has any length (> 1e-4 squared) or it auto-rotates on enter;
// then flags: 0x800 (the per-tick update) only for +0x195/+0x196 objects, and 8 for every one;
// then the "RotationRpm" sound from the template (createSoundFromTemplate - the dedicated server's
// 12-byte stub answers 0) and, when there is one, its volume and pitch evaluators pointed at
// +0x18C and its parent set to this object's ISoundNode.
RotationalBundle::RotationalBundle(const RotationalBundleTemplate* objectTemplate)
    : Bundle(objectTemplate)
{
    m_currentRotation.x = m_currentRotation.y = m_currentRotation.z = 0.0f;                    // +0x140
    m_currentRotationalSpeed.x = m_currentRotationalSpeed.y = m_currentRotationalSpeed.z = 0.0f; // +0x14C
    m_rotationInput.x = m_rotationInput.y = m_rotationInput.z = 0.0f;                                               // +0x158
    m_excessInput.x = m_excessInput.y = m_excessInput.z = 0.0f;                                               // +0x164
    m_forwardAxis.x = 0.0f;                                                                    // +0x170
    m_forwardAxis.z = 1.0f;                                                                    // +0x178
    m_forwardAxis.y = 0.0f;
    m_restoreRotation.x = m_restoreRotation.y = m_restoreRotation.z = 0.0f;                    // +0x17C
    m_inputRegulateAmount = 0.5f;                                                              // +0x198
    m_rotationSound = 0;                                                                       // +0x1A0
    m_soundValue = 0.0f;                                                                       // +0x18C
    m_dword190 = 0;
    m_regulate.x = 0.0f;                                                                       // +0x1A4
    m_byte188 = true;                                                                          // +0x188
    m_byte189 = false;
    m_byte194 = false;
    m_autoRotating = false;                                                                    // +0x195
    m_rotatesContinuously = false;                                                             // +0x196
    m_regulate.y = 0.0f;                                                                       // +0x1A8
    m_regulate.z = 0.0f;                                                                       // +0x1AC
    m_inputOwner = 0;                                                                          // (untouched in the ELF)
    const Vec3& continuous = objectTemplate->m_continousRotationSpeed;                         // template +0x1D0
    if (continuous.x * continuous.x + continuous.y * continuous.y + continuous.z * continuous.z > 0.000099999997f
        || objectTemplate->m_startAutoRotateOnEnter)                                           // template +0x1F4
        m_rotatesContinuously = true;
    if (m_autoRotating || m_rotatesContinuously)
        updateFlags(0x800u, 0);
    updateFlags(8u, 0);
    m_rotationSound = static_cast<ISound*>(objectTemplate->createSoundFromTemplate(std::string("RotationRpm"), this));
    if (m_rotationSound)
    {
        m_rotationSound->setVolumeEvaluator(0, &m_soundValue);                                 // +0x44
        m_rotationSound->setPitchEvaluator(0, &m_soundValue);                                  // +0x4C
        m_rotationSound->setParent(static_cast<ISoundNode*>(this));                            // +0x3C
    }
}

// / 0x81EA140 / 0x81EA190: the vptrs, the sound released (+0x04), ~Bundle.
RotationalBundle::~RotationalBundle()
{
    if (m_rotationSound)
        m_rotationSound->release();
}

// Bundle::init(); restoreRotationOnExit -> remember the current rotation; a
// continuous rotation speed of any length -> the root object takes flag 0x4000000.
void RotationalBundle::init()
{
    Bundle::init();
    const RotationalBundleTemplate* t = static_cast<const RotationalBundleTemplate*>(m_template);
    if (t->m_restoreRotationOnExit)                                                            // template +0x1EB
        m_restoreRotation = getCurrentRotation();                                              // vslot +0x184
    const Vec3& c = t->m_continousRotationSpeed;
    if (std::sqrt(c.x * c.x + c.y * c.y + c.z * c.z) > 1.1920929e-07f)
        getRootParent(this)->updateFlags(0x4000000u, 0);                                       // root's +0x60
}

// / 0x81EA220, 192 B - the object-cache pair: enable re-applies the current
// rotation; disable stops the sound and puts the whole own block back to the ctor's values, then
// SimpleObject::disableCachedObject (skipping Bundle's).
void RotationalBundle::enableCachedObject()
{
    Bundle::enableCachedObject();
    applyRotation(m_currentRotation);
}

void RotationalBundle::disableCachedObject()
{
    if (m_rotationSound)
        m_rotationSound->stop();                                                               // +0x30
    m_currentRotation.x = m_currentRotation.y = m_currentRotation.z = 0.0f;
    m_currentRotationalSpeed.x = m_currentRotationalSpeed.y = m_currentRotationalSpeed.z = 0.0f;
    m_rotationInput.x = m_rotationInput.y = m_rotationInput.z = 0.0f;
    m_excessInput.x = m_excessInput.y = m_excessInput.z = 0.0f;
    m_forwardAxis.x = 0.0f;
    m_byte188 = true;
    m_byte189 = false;
    m_forwardAxis.y = 0.0f;
    m_forwardAxis.z = 1.0f;
    m_soundValue = 0.0f;
    m_dword190 = 0;
    m_byte194 = false;
    m_inputRegulateAmount = 0.5f;
    SimpleObject::disableCachedObject();
}

// Applies negative Z-X-Y rotation around the configured pivot, then updates the bundle transform.
void RotationalBundle::applyRotation(const Vec3& rotation)
{
    const RotationalBundleTemplate* t = static_cast<const RotationalBundleTemplate*>(m_template);
    const Math::Vec3& pivot = t->m_pivotPosition;                                  // template +404

    if (std::sqrt(pivot.x * pivot.x + pivot.y * pivot.y + pivot.z * pivot.z) > 0.0)
    {
        Mat4 rotated = Mat4::getIdentity();
        setRotationZXY(rotated, -rotation.z, -rotation.y, -rotation.x);
        rotated.m[12] = pivot.x;                     // the rotation frame sits AT the pivot
        rotated.m[13] = pivot.y;
        rotated.m[14] = pivot.z;

        Mat4 unpivot = Mat4::getIdentity();
        unpivot.m[12] = -pivot.x;                    // and the operand is pulled back first
        unpivot.m[13] = -pivot.y;
        unpivot.m[14] = -pivot.z;

        Mat4 recentred;
        recentred.mult(unpivot, rotated);
        Mat4 combined;
        combined.mult(recentred, *getBundleTransformation());
        setRelativeTransformation(combined);         // slot +0xEC
    }
    else
    {
        Mat4 rotated = Mat4::getIdentity();
        setRotationZXY(rotated, -rotation.z, -rotation.y, -rotation.x);
        Mat4 combined;
        combined.mult(rotated, *getBundleTransformation());
        setRelativeTransformation(combined);
    }
}

// Compensates yaw for parent rotation relative to the root before applying the local rotation.
void RotationalBundle::applyStabilizedRotation()
{
    const RotationalBundleTemplate* t = static_cast<const RotationalBundleTemplate*>(m_template);
    IObject* const root = getRootParent(this);
    float stabilization = 0.0f;
    Vec3 rotation;
    rotation.x = m_currentRotation.x;
    rotation.y = m_currentRotation.y;
    rotation.z = m_currentRotation.z;

    if (t->m_automaticYawStabilization)                                            // +0x1EC
    {
        IObject* const parent = const_cast<IObject*>(getParent());
        if (root && parent != root)
        {
            {
                const Mat4& parentAbs = parent->getAbsoluteTransformation();
                const Mat4& rootAbs   = root->getAbsoluteTransformation();
                const float zDot = rootAbs.m[8]  * parentAbs.m[8]
                                 + rootAbs.m[9]  * parentAbs.m[9]
                                 + rootAbs.m[10] * parentAbs.m[10];
                const Mat4& parentAbs2 = parent->getAbsoluteTransformation();      // refetched, as shipped
                const Mat4& rootAbs2   = root->getAbsoluteTransformation();
                float xDot = rootAbs2.m[0] * parentAbs2.m[8]
                           + rootAbs2.m[1] * parentAbs2.m[9]
                           + rootAbs2.m[2] * parentAbs2.m[10];
                if (xDot < -1.0f)      xDot = -1.0f;
                else if (xDot > 1.0f)  xDot = 1.0f;
                stabilization = std::asin(xDot) * 57.295776f;                      // degrees
                if (zDot < 0.0f)
                    stabilization = 180.0f - stabilization;
            }
        }
        rotation.x -= stabilization;
    }
    applyRotation(rotation);
}

// with automaticReset, each axis of the current rotation is clamped into
// [min, max] (max tested first); then applyStabilizedRotation through the vtable.
void RotationalBundle::setState()
{
    const RotationalBundleTemplate* t = static_cast<const RotationalBundleTemplate*>(m_template);
    if (t->m_automaticReset)                                                                   // template +0x1E8
    {
        if (m_currentRotation.x > t->m_maxRotation.x)      m_currentRotation.x = t->m_maxRotation.x;
        else if (t->m_minRotation.x > m_currentRotation.x) m_currentRotation.x = t->m_minRotation.x;
        if (m_currentRotation.y > t->m_maxRotation.y)      m_currentRotation.y = t->m_maxRotation.y;
        else if (t->m_minRotation.y > m_currentRotation.y) m_currentRotation.y = t->m_minRotation.y;
        if (m_currentRotation.z > t->m_maxRotation.z)      m_currentRotation.z = t->m_maxRotation.z;
        else if (t->m_minRotation.z > m_currentRotation.z) m_currentRotation.z = t->m_minRotation.z;
    }
    applyStabilizedRotation();                                                 // vslot +0x198
}

// handleMessage - 0x81EBC20, 271 B. Message 3 (a player left): auto-rotation off when the template
// starts it on enter, +0x158 zeroed, and with restoreRotationOnExit the remembered rotation is
// set and applied; then (as for message 24) updateRotationalSound(false, 0). Message 2 (a player
// entered) with startAutoRotateOnEnter: auto-rotation on, +0x158 zeroed. Every message then goes
// to Bundle::handleMessage.
void RotationalBundle::handleMessage(TemplateMessage message, IPlayer* player, bool flag)
{
    const RotationalBundleTemplate* t = static_cast<const RotationalBundleTemplate*>(m_template);
    const int m = static_cast<int>(message);
    if (m == 3)
    {
        if (t->m_startAutoRotateOnEnter)
            m_autoRotating = false;                                                            // +0x195
        m_rotationInput.x = m_rotationInput.y = m_rotationInput.z = 0.0f;
        if (t->m_restoreRotationOnExit)
        {
            setCurrentRotation(m_restoreRotation);                                             // vslot +0x180
            applyRotation(m_currentRotation);
        }
        updateRotationalSound(false, 0.0f);                                                    // [FRONTIER]
    }
    else if (m == 24)
    {
        updateRotationalSound(false, 0.0f);                                                    // [FRONTIER]
    }
    else if (m == 2 && t->m_startAutoRotateOnEnter)
    {
        m_autoRotating = true;
        m_rotationInput.x = m_rotationInput.y = m_rotationInput.z = 0.0f;
    }
    Bundle::handleMessage(message, player, flag);
}

// a non-empty networkableInfo on the template -> setNetworkableComp with
// CID_RotationalBundleNetworkable (39001 - RW-6: gated out of the registry until the class lands).
void RotationalBundle::initNetworkableComp()
{
    if (m_template && m_template->getNetworkableInfo().compare("") != 0)
        setNetworkableComp(this, CID_RotationalBundleNetworkable);
}

// Calculates one axis's angle from input and elapsed time, then clamps or wraps it to template limits.
// The old rotation model also applies reset, banking and acceleration rules.
namespace {
    // The TU's static initialiser (0x81EC990) - the two neighbours of the sound var above.
    int*   const g_rotUseOld               = hfe::Vars::getInt("rot-use-old", 1);                       // ds:0x8AAE14C
    float* const g_rotationalMaxSpeedMod   = hfe::Vars::getFloat("rotationalbundle-max-speed-mod", 30.0f); // ds:0x8AAE154

    // The ELF addresses every Vec3 as base + 4 * axis; x, y, z are contiguous floats.
    inline float&       axisOf(Vec3& v, int axis)             { return (&v.x)[axis]; }
    inline const float& axisOf(const Vec3& v, int axis)       { return (&v.x)[axis]; }
}

void RotationalBundle::calculateAndClipAngle(const RotationalBundleTemplate* t, int axis, float dt)
{
    const float accel   = axisOf(t->m_acceleration, axis);              // +0x1B8
    const float deAccel = axisOf(t->m_deAcceleration, axis);            // +0x1C4
    const float contRot = axisOf(t->m_continousRotationSpeed, axis);    // +0x1D0

    if (accel == 0.0f && contRot == 0.0f)
        return;

    float& rot    = axisOf(m_currentRotation, axis);                    // +0x140
    float& speed  = axisOf(m_currentRotationalSpeed, axis);             // +0x14C
    float& excess = axisOf(m_excessInput, axis);                        // +0x164

    // the input, raw or through the bank
    float input;
    if (t->m_rememberExcessInput && excess * axisOf(m_rotationInput, axis) >= 0.0f)   // same sign
    {
        excess += axisOf(m_rotationInput, axis);
        if (excess > 40.0f)  excess = 40.0f;
        if (excess < -40.0f) excess = -40.0f;
        input = excess;
        if (input > 1.0f)       input = 1.0f;
        else if (input < -1.0f) input = -1.0f;
        excess -= input;
    }
    else
    {
        input  = axisOf(m_rotationInput, axis);
        excess = 0.0f;
    }

    // the snap: rotation to zero when not actively driven
    if (t->m_snapToZeroOnNoInput)
    {
        if ((input < 0.05f && rot > 0.0f) || (input > -0.05f && rot < 0.0f))
            rot = 0.0f;
    }

    // THE OLD model - the default.
    if (*g_rotUseOld == 1)
    {
        float target;
        if (t->m_automaticReset)
        {
            if (accel > 0.0f)       target =  input * axisOf(t->m_maxRotation, axis);   // +0x188
            else if (accel < 0.0f)  target = -input * axisOf(t->m_maxRotation, axis);
            else                    target = 0.0f;
        }
        else
        {
            if (accel > 0.0f)       target =  input * axisOf(t->m_maxSpeed, axis);      // +0x1A0
            else if (accel < 0.0f)  target = -input * axisOf(t->m_maxSpeed, axis);
            else                    target = 0.0f;
        }

        if (t->m_capMaxSpeed)                                                       // +0x210
        {
            const float cap = axisOf(t->m_maxSpeed, axis) * *g_rotationalMaxSpeedMod;
            if (target > cap)  target = cap;
            if (target < -cap) target = -cap;
        }

        if (t->m_automaticReset)
        {
            // The input positions the bundle: chase the target rotation.
            if (target > rot)
            {
                rot += fabsf(accel) * dt;
                if (rot > target) rot = target;
            }
            else if (rot > target)
            {
                rot -= fabsf(t->m_useDeAcceleration ? deAccel : accel) * dt;
                if (rot < target) rot = target;
            }
        }
        else
        {
            // The input drives the bundle: ramp the speed, then integrate.
            if (target > speed)
            {
                speed += fabsf(accel) * dt;
                if (speed > target) speed = target;
            }
            else if (speed > target)
            {
                speed -= fabsf(t->m_useDeAcceleration ? deAccel : accel) * dt;
                if (speed < target) speed = target;
            }
            rot += speed * dt;                       // two stores in the ELF; the first is dead
            rot += contRot * dt;
        }

        // The old clip: wrap only when both limits are exactly zero, else clamp. No epsilon.
        const float maxRot = axisOf(t->m_maxRotation, axis);
        const float minRot = axisOf(t->m_minRotation, axis);                    // +0x17C
        if (maxRot == 0.0f && minRot == 0.0f)
        {
            if (rot > 180.0f)       rot -= 360.0f;
            else if (rot < -180.0f) rot += 360.0f;
            m_byte189 = false;
            return;
        }
        if (rot > maxRot) { rot = maxRot; return; }         // +0x189 NOT cleared when clipped
        if (rot < minRot) { rot = minRot; return; }
        m_byte189 = false;
        return;
    }

    // THE NEW model - rot-use-old 0.
    bool synthesised = false;
    if (t->m_automaticReset)
    {
        // No real input and the bundle is off-centre: synthesise a unit input toward centre.
        if (fabsf(input) <= FLT_EPSILON && fabsf(rot) > FLT_EPSILON)
        {
            input = (rot > 0.0f) ? -1.0f : 1.0f;
            synthesised = true;
        }
    }

    float target;
    if (accel > 0.0f)       target =  input * axisOf(t->m_maxSpeed, axis);
    else if (accel < 0.0f)  target = -input * axisOf(t->m_maxSpeed, axis);
    else                    target = 0.0f;

    // The 4-way ramp. Speeding up toward the target uses deAcceleration only when the rotation is on
    // the far side of centre (the bundle is coming home); slowing down uses it whenever enabled.
    if (target >= 0.0f)
    {
        if (target > speed)
        {
            const bool useDe = t->m_useDeAcceleration && rot < 0.0f;
            speed += fabsf(useDe ? deAccel : accel) * dt;
            if (speed > target) speed = target;
        }
        else if (speed > target)
        {
            speed -= fabsf(t->m_useDeAcceleration ? deAccel : accel) * dt;
            if (speed < target) speed = target;
        }
    }
    else
    {
        if (speed > target)
        {
            const bool useDe = t->m_useDeAcceleration && rot > 0.0f;
            speed -= fabsf(useDe ? deAccel : accel) * dt;
            if (speed < target) speed = target;
        }
        else if (speed < target)
        {
            speed += fabsf(t->m_useDeAcceleration ? deAccel : accel) * dt;
            if (speed > target) speed = target;
        }
    }

    // Integrate - and with a synthesised auto-reset input, stop dead at the zero-crossing.
    if (t->m_automaticReset && synthesised)
    {
        const float newRot = rot + speed * dt + contRot * dt;
        if ((newRot > 0.0f) != (rot >= 0.0f))       // crossed centre
        {
            speed = 0.0f;
            rot   = 0.0f;
        }
        else
        {
            rot = newRot;
        }
    }
    else
    {
        rot += speed * dt;
        rot += contRot * dt;
    }

    // The new clip: a limit within FLT_EPSILON of zero is "no limit on that side"; wrap only when the
    // MIN limit is absent. +0x189 is cleared on every unclipped path.
    const float maxRot = axisOf(t->m_maxRotation, axis);
    if (fabsf(maxRot) > FLT_EPSILON)
    {
        if (rot > maxRot) rot = maxRot;
        else              m_byte189 = false;
    }
    const float minRot = axisOf(t->m_minRotation, axis);
    if (fabsf(minRot) > FLT_EPSILON)
    {
        if (rot < minRot) rot = minRot;
        else              m_byte189 = false;
        return;
    }
    if (rot > 180.0f)       rot -= 360.0f;
    else if (rot < -180.0f) rot += 360.0f;
    m_byte189 = false;
}

// Updates rotation sound from motion, subject to landing-gear and armor state.
// Smooths its level and uses separate start and stop thresholds.
namespace {
    // The TU's static initialiser (0x81EC990): Vars::getFloat("rotationalbundle-sound-smooth-time",
    // 0.15) -> ds:0x8AAE150 (its neighbours rot-use-old / the unnamed 30.0 var belong to the
    // [FRONTIER] rotation machine).
    float* const g_rotationalSoundSmoothTime = hfe::Vars::getFloat("rotationalbundle-sound-smooth-time", 0.15f);
}

void RotationalBundle::updateRotationalSound(bool fromSpeed, float level)
{
    if (!m_rotationSound || m_template->getClassID() == CID_LandingGearTemplate)
        return;
    if (fromSpeed || level != 0.0f)
    {
        IObjectComponent* armor = 0;
        for (IObject* walk = this; walk; walk = walk->getParent())
        {
            armor = object_detail::LayoutAccess::armorComponent(walk);               // +0x3C
            if (armor)
                break;
        }
        if (armor && static_cast<IArmorGLComp*>(armor)->isDestroyed())
            return;
    }
    float target = 0.0f;
    if (fromSpeed)
    {
        if (!m_byte189)
        {
            const RotationalBundleTemplate* t = static_cast<const RotationalBundleTemplate*>(m_template);
            const float a[3] = {
                std::fabs(t->m_continousRotationSpeed.x) + std::fabs(m_currentRotationalSpeed.x),
                std::fabs(t->m_continousRotationSpeed.y) + std::fabs(m_currentRotationalSpeed.y),
                std::fabs(t->m_continousRotationSpeed.z) + std::fabs(m_currentRotationalSpeed.z) };
            int axis;
            if (a[0] > a[1] && a[0] > a[2]) axis = 0;
            else                            axis = (a[1] <= a[2]) ? 2 : 1;
            const float* maxSpeed   = &t->m_maxSpeed.x;
            const float* continuous = &t->m_continousRotationSpeed.x;
            const float range = std::fabs(maxSpeed[axis]) + std::fabs(continuous[axis]);
            if (range > 0.0f)
                target = a[axis] / (range * 3.0f);
        }
    }
    else
    {
        if (level == 0.0f)
        {
            m_soundValue = 0.0f;                                                     // +0x18C
            m_dword190 = 0;
        }
        target = std::fabs(level);
    }
    if (!m_byte194)                                                                  // +0x194
    {
        if (target > 0.000099999997f)
        {
            m_rotationSound->play();                                                 // +0x10
            m_byte194 = true;
            m_soundValue = 0.000099999997f;
        }
    }
    if (m_byte194)
    {
        float v = smooth_in_out(m_soundValue, target, reinterpret_cast<float*>(&m_dword190),
                                *g_rotationalSoundSmoothTime, 0.033333335f);
        if (v < 0.0f) v = 0.0f; else if (v > 1.0f) v = 1.0f;
        m_soundValue = v;
        if (v <= 0.000049999999f)
        {
            m_rotationSound->stop();                                                 // +0x30
            m_byte194 = false;
            m_soundValue = 0.0f;
            m_dword190 = 0;
        }
    }
}

// The four accessors (56 / 13 B) and the two ISoundNode slots.
void RotationalBundle::setCurrentRotation(const Vec3& rotation)        { m_currentRotation = rotation; }
const Vec3& RotationalBundle::getCurrentRotation() const               { return m_currentRotation; }
void RotationalBundle::setCurrentRotationalSpeed(const Vec3& speed)    { m_currentRotationalSpeed = speed; }
const Vec3& RotationalBundle::getCurrentRotationalSpeed() const        { return m_currentRotationalSpeed; }
const Mat4& RotationalBundle::getTransform()                           { return getAbsoluteTransformation(); }   // vslot +0x84
const Math::Vec3* RotationalBundle::getVelocity()                      { return 0; }

// Adds the looping 3D RotationRpm sound with a 0.5 minimum distance.
void RotationalBundle::initSoundTemplates(RotationalBundleTemplate* objectTemplate)
{
    hfe::SoundObjectTemplate* s =
        static_cast<hfe::SoundObjectTemplate*>(objectTemplate->addSoundTemplate("RotationRpm"));
    s->impl()->setIs3dSound(true);
    s->impl()->setLoopCount(0);
    s->impl()->setMinDistance(0.5f);
}


// Read one content-mapped axis. An index above 63, or an input the control map has not bound,
// reads as a hard zero - the same rule Soldier::handlePlayerInput and Camera::handlePlayerInput use,
// and the third independent sighting of "the bit IS the index".
inline float readMappedInput(const io::PlayerInput& in, io::PlayerInputMap index)
{
    const int i = static_cast<int>(index);
    if (i > 63)
        return 0.0f;
    return ((in.mappingFlags >> i) & 1ull) ? in.inputs[i] : 0.0f;
}

// Returns whether the bundle can sleep after half a second without an occupant, input or rotation.
// Continuously rotating bundles remain awake.
bool RotationalBundle::goToSleep(float deltaTime)
{
    if (m_rotatesContinuously)                                      // +0x196
        return false;

    IPlayerControlObject* pco = 0;
    for (IObject* node = this; node; node = node->getParent())
    {
        pco = static_cast<IPlayerControlObject*>(
            node->queryInterface(IID_IPlayerControlObject));
        if (pco)
            break;
    }
    if (pco && pco->isOccupied())                                   // ipco +0x18
        return false;

    const RotationalBundleTemplate* const t =
        static_cast<const RotationalBundleTemplate*>(m_template);

    bool idle = false;
    if (!m_rotatesContinuously)
        idle = !m_autoRotating;                                     // +0x195

    if (idle
        && std::fabs(m_rotationInput.x) <= 1.1920929e-07f
        && std::fabs(m_rotationInput.y) <= 1.1920929e-07f
        && std::fabs(m_rotationInput.z) <= 1.1920929e-07f)
    {
        // "still turning?" is the continuous speed plus THE current speed, per axis, summed as
        // squares - so a bundle coasting to a stop keeps its tick until it really is stopped.
        const float x = std::fabs(t->m_continousRotationSpeed.x) + std::fabs(m_currentRotationalSpeed.x);
        const float y = std::fabs(m_currentRotationalSpeed.y) + std::fabs(t->m_continousRotationSpeed.y);
        const float z = std::fabs(t->m_continousRotationSpeed.z) + std::fabs(m_currentRotationalSpeed.z);
        if (y * y + x * x + z * z < 0.0000000099999991f)
        {
            m_inputRegulateAmount -= deltaTime;                     // +0x198 here a sleep countdown
            if (m_inputRegulateAmount <= 0.0f)
            {
                m_inputRegulateAmount = 0.0f;
                if ((getFlags() & 0x800u) != 0)
                {
                    updateFlags(0, 0x800u);                         // off the tick
                    return true;
                }
            }
        }
        else
        {
            m_inputRegulateAmount = 0.5f;
        }
        return false;
    }

    // something is happening - re-arm the countdown, and wake the object unless it is a camera or an
    // engine (each of those is driven by its owner, never by the tick).
    m_inputRegulateAmount = 0.5f;
    if ((getFlags() & 0x800u) == 0
        && t->getClassID() != CID_CameraTemplate
        && t->getClassID() != CID_EngineTemplate)
        updateFlags(0x800u, 0);
    return false;
}

// Ticks Bundle components and updates parented rotation when simulation and sleep state permit it.
// Applies changed angles and updates rotation sound.
void RotationalBundle::handleUpdate(float deltaTime, unsigned int flags)
{
    SimpleObject::handleUpdate(deltaTime, flags);

    if (m_autoRotating && (flags & 4u) != 0)                        // +0x195
        return;

    const RotationalBundleTemplate* const t =
        static_cast<const RotationalBundleTemplate*>(m_template);
    if (goToSleep(deltaTime))
        return;

    if (m_autoRotating || t->m_debugAutoRotate)                     // template +0x1F5
    {
        m_rotationInput.x = 1.0f;
        m_rotationInput.y = 1.0f;
        m_rotationInput.z = 1.0f;
    }

    if (!getParent())
    {
        updateRotationalSound(true, 0.0f);
        return;
    }

    if (m_byte188)
    {
        m_byte188 = false;
        applyRotation(m_currentRotation);
        const Mat4& transform = getAbsoluteTransformation();
        m_forwardAxis.x = transform.m[8];
        m_forwardAxis.y = transform.m[9];
        m_forwardAxis.z = transform.m[10];
    }

    // nothing below runs for a bundle whose template cannot rotate on any axis
    if (!(std::fabs(t->m_maxSpeed.x) > 1.1920929e-07f
          || std::fabs(t->m_maxSpeed.y) > 1.1920929e-07f
          || std::fabs(t->m_maxSpeed.z) > 1.1920929e-07f))
        return;

    if (t->m_lesserYawAtSpeedSet)                                   // template +0x1EF
    {
        IPhysicsNode* const physics = static_cast<IPhysicsNode*>(
            object_detail::LayoutAccess::physicsNode(getRootParent(this)));      // root +0x44
        if (physics)
        {
            const Vec3& speed = physics->getPositionalSpeed();                   // +0x60
            const float speedSq = speed.x * speed.x + speed.y * speed.y + speed.z * speed.z;
            // the ELF calls an accessor here three times, never cached; this tree has no such
            // accessor on the template, so the field is read three times instead - same reads.
            if (speedSq > t->m_lesserYawAtSpeed * t->m_lesserYawAtSpeed)
                m_rotationInput.x = t->m_lesserYawAtSpeed
                                  * std::sqrt(std::sqrt(speedSq))
                                  * m_rotationInput.x;
        }
        else
        {
            Debug(DEBUGTYPE_6, "Game/Objects/Base/RotationalBundle.cpp", 540)
                << "Object with LesserYawAtSpeed set needs root with physicsNode!";
        }
    }

    m_byte189 = true;
    const Vec3 before = m_currentRotation;

    if (t->m_maxSpeed.x != 0.0f) calculateAndClipAngle(t, 0, deltaTime);
    if (t->m_maxSpeed.y != 0.0f) calculateAndClipAngle(t, 1, deltaTime);
    if (t->m_maxSpeed.z != 0.0f) calculateAndClipAngle(t, 2, deltaTime);

    if (t->m_automaticYawStabilization)                             // template +0x1EC
    {
        applyStabilizedRotation();
    }
    else
    {
        bool unchanged = false;
        if (std::fabs(m_currentRotation.x - before.x) <= 1.1920929e-07f
            && std::fabs(m_currentRotation.y - before.y) <= 1.1920929e-07f)
            unchanged = std::fabs(m_currentRotation.z - before.z) <= 1.1920929e-07f;
        if (!unchanged)
            applyRotation(m_currentRotation);
    }

    const Mat4& transform = getAbsoluteTransformation();
    m_forwardAxis.x = transform.m[8];
    m_forwardAxis.y = transform.m[9];
    m_forwardAxis.z = transform.m[10];

    updateRotationalSound(true, 0.0f);
}

// Writes clamped pitch and yaw regulation to out using local angular speed and deltaTime.
// Clears out when root physics or regulation is unavailable; active regulation requires nonzero deltaTime.
void RotationalBundle::updateRegulate(Vec3& out, float deltaTime) const
{
    out.x = 0.0f;
    out.y = 0.0f;
    out.z = 0.0f;

    IPhysicsNode* const physics = static_cast<IPhysicsNode*>(
        object_detail::LayoutAccess::physicsNode(getRootParent(this)));          // root +0x44
    if (!physics)
        return;

    const RotationalBundleTemplate* const t =
        static_cast<const RotationalBundleTemplate*>(getTemplate());             // +0x14
    if (t->m_regulateYaw.x == 0.0f && t->m_regulatePitch.x == 0.0f && t->m_regulateRoll.x == 0.0f)
        return;

    const float scale[3]     = { t->m_regulateYaw.x, t->m_regulatePitch.x, t->m_regulateRoll.x };
    const float perSecond[3] = { t->m_regulateYaw.y, t->m_regulatePitch.y, t->m_regulateRoll.y };

    Mat4 relative = Mat4::getIdentity();     // mult leaves the translation row alone - see Mat4.h
    relative.mult(getAbsoluteTransformation(),
                  getRootParent(this)->getInvAbsoluteTransformation());

    const Vec3& rotationalSpeed = physics->getRotationalSpeed();                 // +0x64
    const float sx = rotationalSpeed.x, sy = rotationalSpeed.y, sz = rotationalSpeed.z;

    // the root's rotation, in this bundle's frame
    const float lx = relative.m[0] * sx + relative.m[4] * sy + relative.m[8]  * sz;
    const float ly = relative.m[1] * sx + relative.m[5] * sy + relative.m[9]  * sz;
    const float lz = relative.m[2] * sx + relative.m[6] * sy + relative.m[10] * sz;

    const float inverseDt = 1.0f / deltaTime;
    const float ax = lx * inverseDt, ay = ly * inverseDt, az = lz * inverseDt;

    const Mat4& rootTransform = getRootParent(this)->getAbsoluteTransformation();

    // the yaw/pitch swap: [0] takes column 1, [1] takes column 0, [2] takes column 2
    float speed[3];
    speed[0] = lx * rootTransform.m[4] + ly * rootTransform.m[5] + lz * rootTransform.m[6];
    speed[1] = lx * rootTransform.m[0] + ly * rootTransform.m[1] + lz * rootTransform.m[2];
    speed[2] = lx * rootTransform.m[8] + ly * rootTransform.m[9] + lz * rootTransform.m[10];

    float accel[3];
    accel[0] = ax * rootTransform.m[4] + ay * rootTransform.m[5] + az * rootTransform.m[6];
    accel[1] = ax * rootTransform.m[0] + ay * rootTransform.m[1] + az * rootTransform.m[2];
    accel[2] = ax * rootTransform.m[8] + ay * rootTransform.m[9] + az * rootTransform.m[10];

    float* const axis = &out.x;
    for (int i = 0; i < 3; ++i)
    {
        const float value = (accel[i] * perSecond[i] + speed[i]) * scale[i];
        axis[i] = value < -1.0f ? -1.0f : (value > 1.0f ? 1.0f : value);
    }
}

void RotationalBundle::handlePlayerInput(IPlayer* player, const io::PlayerInput& input,
                                         float dt, unsigned int flags)
{
    // the claim-the-controls arm - see above
    if (player && m_inputOwner == player)
    {
        io::PlayerInput& victim = const_cast<io::PlayerInput&>(input);
        for (int i = 0; i <= 63; ++i)
            victim.inputs[i] = 0.0f;                         // the caller'S array
        m_rotationInput.x = 0.0f;
        m_rotationInput.y = 0.0f;
        m_rotationInput.z = 0.0f;
        return;
    }

    IPlayerControlObject* pco = 0;
    for (IObject* node = this; node; node = node->getParent())
    {
        pco = static_cast<IPlayerControlObject*>(node->queryInterface(IID_IPlayerControlObject));
        if (pco)
            break;
    }
    IObject* pcoObject = static_cast<IObject*>(pco->queryInterface(IID_IObject));   // pco unchecked

    const RotationalBundleTemplate* t =
        static_cast<const RotationalBundleTemplate*>(getTemplate());

    // a root control object regulates; anything else is zeroed
    const bool pcoIsRoot = (pcoObject && !pcoObject->getParent());
    if (pcoIsRoot)
    {
        updateRegulate(m_regulate, dt);
    }
    else
    {
        m_regulate.x = 0.0f;
        m_regulate.y = 0.0f;
        m_regulate.z = 0.0f;
    }

    // the content-mapped axes
    m_rotationInput.x = readMappedInput(input, t->getInputToYaw());
    m_rotationInput.y = readMappedInput(input, t->getInputToPitch());
    m_rotationInput.z = readMappedInput(input, t->getInputToRoll());

    if (pcoIsRoot || !(flags & 4u))
    {
        float* in  = &m_rotationInput.x;
        float* reg = &m_regulate.x;
        for (int k = 0; k < 3; ++k)
        {
            if (in[k] == 0.0f)
            {
                in[k] = reg[k];                              // no input at all -> the regulate
                continue;
            }
            const float signIn  = (in[k]  > 0.0f) ? 1.0f : ((in[k]  < 0.0f) ? -1.0f : 0.0f);
            const float signReg = (reg[k] > 0.0f) ? 1.0f : ((reg[k] < 0.0f) ? -1.0f : 0.0f);
            // same direction AND larger - it can help, never fight
            if (signIn == signReg && fabsf(reg[k]) > fabsf(in[k]))
                in[k] = reg[k];
        }
    }

    // flag 0x800 = byte +0x05, bit 3 of the flags word at +0x04
    if (!(getFlags() & 0x800u))
    {
        const unsigned int cid = t->getClassID();
        if (cid != CID_CameraTemplate && cid != CID_EngineTemplate)   // both exempt
        {
            updateFlags(2048u, 0u);
            m_inputRegulateAmount = 0.5f;                    // +0x198
        }
    }
}

// (983 B), PE dedicated 0x622E40 (401 B). The retail client
// calls the PCO interface's +0x15C slot at BF2.exe 0x5C0351 from the local-input
// step at 0x5C0260; PCO then forwards to its collected rotational children.
// Only a parented control object takes this pre-pass. Missing PCO/object interfaces
// are unchecked in both images. setInput preserves the existing mapping mask.
void RotationalBundle::preHandlePlayerInput(io::PlayerInput& input, float dt) const
{
    IPlayerControlObject* pco = 0;
    for (IObject* node = const_cast<RotationalBundle*>(this); node; node = object_detail::LayoutAccess::parent(node))
    {
        pco = static_cast<IPlayerControlObject*>(node->queryInterface(IID_IPlayerControlObject));
        if (pco)
            break;
    }
    IObject* const pcoObject = static_cast<IObject*>(pco->queryInterface(IID_IObject));
    if (!object_detail::LayoutAccess::parent(pcoObject))
        return;

    static Vec3 regulate = {};
    updateRegulate(regulate, dt);
    if (!(regulate.z * regulate.z + regulate.y * regulate.y + regulate.x * regulate.x > 0.0f))
        return;
    const RotationalBundleTemplate* const t =
        static_cast<const RotationalBundleTemplate*>(m_template);
    const io::PlayerInputMap axes[] = {t->getInputToYaw(), t->getInputToPitch(), t->getInputToRoll()};
    const float* const values = &regulate.x;
    for (int i = 0; i < 3; ++i)
    {
        if (axes[i] == io::PINone || values[i] == 0.0f)
            continue;
        const float current = readMappedInput(input, axes[i]);
        const float inputSign = current > 0.0f ? 1.0f : (current < 0.0f ? -1.0f : 0.0f);
        const float regulateSign = values[i] > 0.0f ? 1.0f : (values[i] < 0.0f ? -1.0f : 0.0f);
        if (current == 0.0f || (inputSign == regulateSign && fabsf(values[i]) > fabsf(current)))
            input.setInput(axes[i], values[i]);
    }
}

} // namespace world
} // namespace hfe
} // namespace dice

