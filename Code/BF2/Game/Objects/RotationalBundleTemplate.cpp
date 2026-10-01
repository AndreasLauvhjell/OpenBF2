// Rotational-bundle defaults, initialization, and script serialization.

#include "RotationalBundleTemplate.h"
#include "../../IO/Stream/IStream.h"
#include <sstream>
#include <cmath>
#include "Game/RotationalBundle.h"                 // the object class - createObject / initSoundTemplates

#include "../../Core/Ids.h"
#include "../../Sound/SoundObjectTemplate.h"

namespace dice {
namespace hfe {
namespace world {


// Zeroes rotation, regulation and speed state and marks each input axis unmapped.
// Trailing alignment bytes are left untouched.
RotationalBundleTemplate::RotationalBundleTemplate()
{
    m_minRotation.x = 0.0f;  m_minRotation.y = 0.0f;  m_minRotation.z = 0.0f;
    m_maxRotation.x = 0.0f;  m_maxRotation.y = 0.0f;  m_maxRotation.z = 0.0f;
    m_pivotPosition.x = 0.0f; m_pivotPosition.y = 0.0f; m_pivotPosition.z = 0.0f;
    m_maxSpeed.x = 0.0f;     m_maxSpeed.y = 0.0f;     m_maxSpeed.z = 0.0f;
    m_maxResetSpeed.x = 0.0f; m_maxResetSpeed.y = 0.0f; m_maxResetSpeed.z = 0.0f;
    m_acceleration.x = 0.0f; m_acceleration.y = 0.0f; m_acceleration.z = 0.0f;
    m_deAcceleration.x = 0.0f; m_deAcceleration.y = 0.0f; m_deAcceleration.z = 0.0f;
    m_continousRotationSpeed.x = 0.0f;   // the engine's own spelling - see the header
    m_continousRotationSpeed.y = 0.0f;
    m_continousRotationSpeed.z = 0.0f;

    // Unmapped input axes use PINone.
    m_inputToYaw   = io::PINone;         // 0x40 - the same unmapped-input code PCO uses
    m_inputToPitch = io::PINone;
    m_inputToRoll  = io::PINone;

    m_automaticReset = false;
    m_snapToZeroOnNoInput = false;
    m_useDeAcceleration = false;
    m_restoreRotationOnExit = false;
    m_automaticYawStabilization = false;
    m_automaticPitchStabilization = false;
    m_rememberExcessInput = false;
    m_lesserYawAtSpeedSet = false;
    m_lesserYawAtSpeed = 0.0f;

    m_startAutoRotateOnEnter = false;
    m_debugAutoRotate = false;

    m_regulateYaw.x = 0.0f;   m_regulateYaw.y = 0.0f;
    m_regulatePitch.x = 0.0f; m_regulatePitch.y = 0.0f;
    m_regulateRoll.x = 0.0f;  m_regulateRoll.y = 0.0f;

    m_capMaxSpeed = false;
    // +0x211..+0x213: NOT written by the ELF ctor. Left alone here too.
}

// (D1): vptr resets + ~BundleTemplate - nothing manual survives the translation.
RotationalBundleTemplate::~RotationalBundleTemplate()
{
}

unsigned int RotationalBundleTemplate::getClassID() const
{
    return CID_RotationalBundleTemplate;
}

// a bare tail-jump. The whole of this class's initialisation is the object side's.
void RotationalBundleTemplate::templateInitialize()
{
    RotationalBundle::initSoundTemplates(this);
}

// the standard one-CID hop into Bundle's chain (a literal tail-jump in the ELF).
void* RotationalBundleTemplate::queryImplementation(unsigned int classId)
{
    if (classId == CID_RotationalBundleTemplate)
        return this;
    return BundleTemplate::queryImplementation(classId);
}

// createObject - 47 B: new(0x1B0) RotationalBundle(this). (the vehicle sitting, part 5 -
// the object class is Game/Objects/Game/RotationalBundle.h).
IObject* RotationalBundleTemplate::createObject() const { return new RotationalBundle(this); }

// ELF 81ECF20 / dedicated PE 5F3840. Vec3 equality is componentwise epsilon;
// its negation deliberately emits unordered values, unlike the scalar > epsilon tests.
bool RotationalBundleTemplate::makeScript(io::IStream* stream)
{
    if (!BundleTemplate::makeScript(stream)) return false;
    std::ostringstream out;
    const float epsilon = 1.1920929e-7f;
    if (!(std::fabs(m_minRotation.x) <= epsilon && std::fabs(m_minRotation.y) <= epsilon && std::fabs(m_minRotation.z) <= epsilon))
        out << "ObjectTemplate.setMinRotation " << m_minRotation.x << '/' << m_minRotation.y << '/' << m_minRotation.z << std::endl;
    if (!(std::fabs(m_maxRotation.x) <= epsilon && std::fabs(m_maxRotation.y) <= epsilon && std::fabs(m_maxRotation.z) <= epsilon))
        out << "ObjectTemplate.setMaxRotation " << m_maxRotation.x << '/' << m_maxRotation.y << '/' << m_maxRotation.z << std::endl;
    if (!(std::fabs(m_pivotPosition.x) <= epsilon && std::fabs(m_pivotPosition.y) <= epsilon && std::fabs(m_pivotPosition.z) <= epsilon))
        out << "ObjectTemplate.setPivotPosition " << m_pivotPosition.x << '/' << m_pivotPosition.y << '/' << m_pivotPosition.z << std::endl;
    if (!(std::fabs(m_maxSpeed.x) <= epsilon && std::fabs(m_maxSpeed.y) <= epsilon && std::fabs(m_maxSpeed.z) <= epsilon))
        out << "ObjectTemplate.setMaxSpeed " << m_maxSpeed.x << '/' << m_maxSpeed.y << '/' << m_maxSpeed.z << std::endl;
    if (!(std::fabs(m_maxResetSpeed.x) <= epsilon && std::fabs(m_maxResetSpeed.y) <= epsilon && std::fabs(m_maxResetSpeed.z) <= epsilon))
        out << "ObjectTemplate.setMaxResetSpeed " << m_maxResetSpeed.x << '/' << m_maxResetSpeed.y << '/' << m_maxResetSpeed.z << std::endl;
    if (!(std::fabs(m_acceleration.x) <= epsilon && std::fabs(m_acceleration.y) <= epsilon && std::fabs(m_acceleration.z) <= epsilon))
        out << "ObjectTemplate.setAcceleration " << m_acceleration.x << '/' << m_acceleration.y << '/' << m_acceleration.z << std::endl;
    if (!(std::fabs(m_deAcceleration.x) <= epsilon && std::fabs(m_deAcceleration.y) <= epsilon && std::fabs(m_deAcceleration.z) <= epsilon))
        out << "ObjectTemplate.setDeAcceleration " << m_deAcceleration.x << '/' << m_deAcceleration.y << '/' << m_deAcceleration.z << std::endl;
    if (!(std::fabs(m_continousRotationSpeed.x) <= epsilon && std::fabs(m_continousRotationSpeed.y) <= epsilon && std::fabs(m_continousRotationSpeed.z) <= epsilon))
        out << "ObjectTemplate.setContinousRotationSpeed " << m_continousRotationSpeed.x << '/' << m_continousRotationSpeed.y << '/' << m_continousRotationSpeed.z << std::endl;
    if (m_inputToYaw != io::PINone) out << "ObjectTemplate.setInputToYaw " << m_inputToYaw << std::endl;
    if (m_inputToPitch != io::PINone) out << "ObjectTemplate.setInputToPitch " << m_inputToPitch << std::endl;
    if (m_inputToRoll != io::PINone) out << "ObjectTemplate.setInputToRoll " << m_inputToRoll << std::endl;
    if (m_automaticReset) out << "ObjectTemplate.setAutomaticReset " << m_automaticReset << std::endl;
    if (m_snapToZeroOnNoInput) out << "ObjectTemplate.snapToZeroOnNoInput " << m_snapToZeroOnNoInput << std::endl;
    if (m_useDeAcceleration) out << "ObjectTemplate.setUseDeAcceleration " << m_useDeAcceleration << std::endl;
    if (m_automaticYawStabilization) out << "ObjectTemplate.automaticYawStabilization " << m_automaticYawStabilization << std::endl;
    if (m_automaticPitchStabilization) out << "ObjectTemplate.automaticPitchStabilization " << m_automaticPitchStabilization << std::endl;
    if (std::fabs(m_lesserYawAtSpeed) > epsilon) out << "ObjectTemplate.setLesserYawAtSpeed " << m_lesserYawAtSpeed << std::endl;
    if (m_rememberExcessInput) out << "ObjectTemplate.rememberExcessInput " << m_rememberExcessInput << std::endl;
    if (m_startAutoRotateOnEnter) out << "ObjectTemplate.startAutoRotateOnEnter 1" << std::endl;
    if (std::fabs(static_cast<double>(m_regulateYaw.y)) + std::fabs(static_cast<double>(m_regulateYaw.x)) > epsilon)
        out << "ObjectTemplate.regulateYaw " << m_regulateYaw.x << '/' << m_regulateYaw.y << std::endl;
    if (std::fabs(static_cast<double>(m_regulatePitch.y)) + std::fabs(static_cast<double>(m_regulatePitch.x)) > epsilon)
        out << "ObjectTemplate.regulatePitch " << m_regulatePitch.x << '/' << m_regulatePitch.y << std::endl;
    if (std::fabs(static_cast<double>(m_regulateRoll.y)) + std::fabs(static_cast<double>(m_regulateRoll.x)) > epsilon)
        out << "ObjectTemplate.regulateRoll " << m_regulateRoll.x << '/' << m_regulateRoll.y << std::endl;
    if (m_restoreRotationOnExit) out << "ObjectTemplate.restoreRotationOnExit " << m_restoreRotationOnExit << std::endl;
    const std::string text = out.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.size())) == text.size();
}

} // namespace world
} // namespace hfe
} // namespace dice
