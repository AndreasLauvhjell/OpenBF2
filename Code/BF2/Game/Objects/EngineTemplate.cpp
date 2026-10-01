// Engine tuning, drivetrain selection, and template serialization.

#include "EngineTemplate.h"
#include "../../IO/Stream/IStream.h"
#include <sstream>

#include <cmath>
#include "Game/Engine.h"                          // createObject's two products
#include "Game/CarEngine.h"

#include "../../Core/Ids.h"
#include "../../Sound/SoundObjectTemplate.h"

namespace dice {
namespace hfe {


namespace world {

// Initializes engine tuning and seeds each authored distribution key before generating its curve.
EngineTemplate::EngineTemplate()
{
    m_refCount = 1;

    // Both distributions start blank.
    for (int i = 0; i < 4; ++i) { m_distributionA.m_authored[i] = 0; m_distributionB.m_authored[i] = 0; }
    m_distributionA.m_field00 = 0;
    m_distributionB.m_field00 = 0;
    m_distributionA.m_samples[0] = 1.0f;         // +0x21C ctor seed
    for (int i = 1; i < 102; ++i) m_distributionA.m_samples[i] = m_distributionA.m_samples[0];
    m_distributionB.m_samples[0] = 1.0f;         // +0x3E0 ctor seed
    for (int i = 1; i < 102; ++i) m_distributionB.m_samples[i] = m_distributionB.m_samples[0];

    // Initialize the represented engine parameters and their authored defaults.
    m_numberOfGears = 1; // +0x3C8
    m_differential = 10.0f;                      // +0x3CC
    m_torque = 60.0f;                            // +0x3D0
    m_gearUp = 0.7f;                             // +0x3D4
    m_gearDown = 0.3f;                           // +0x3D8
    m_gearChangeTime = 1.0f;                     // +0x3DC
    m_noPropellerEffectAtSpeed = 500.0f;         // +0x58C
    m_heatIncrement = -1.0f;                     // +0x594
    m_coolingFactor = 0.0f;                      // +0x598
    m_heatBarType = 4;                           // +0x59C
    m_inverseHeatBar = false;                    // +0x5A0
    m_attackSpeed = 0.0f;                        // +0x5B8
    m_decreaseAngleToZero = false;               // +0x5C0
    m_spinWhenNoEngineInAir = 0.0f;              // +0x5C4
    m_constantRpm = false;                       // +0x5C8
    m_horizontalDampAngle = -1.0f;               // +0x5CC
    m_horizontalDampAngleFactor = 1.0f;           // +0x5D0
    m_dampHorizontalVel = 0.0f;                  // +0x5D4
    m_dampHorizontalVelFactor = 0.0f;            // +0x5D8
    m_decreaseAngleToZeroVerticalVel = 0.0f;      // +0x5DC
    m_decreaseAngleToZeroSpeed = 0.0f;            // +0x5E0
    m_airFlowAffect = 0.0f;                      // +0x5E4
    m_noEffectAtPerpSpeed = 10000.0f;            // +0x5E8
    m_trackTurnLowAmplitude = 2.0f;              // +0x5F0
    m_trackTurnLowOffset = -1.5f;                // +0x5F4
    m_trackTurnHighAmplitude = 1.3f;             // +0x5F8
    m_trackTurnHighOffset = 0.45f;               // +0x5FC
    m_trackTurnAcceleration = 120.0f;            // +0x600
    m_trackTurnSpeed = 1.5f;                     // +0x604
    m_sprintGear1Dampen = 1.0f;                  // +0x608
    m_sprintGear2Dampen = 1.2f;                  // +0x60C
    m_sprintGear3Dampen = 1.5f;                  // +0x610
    m_sprintGear4Dampen = 2.0f;                  // +0x614
    m_sprintGear5Dampen = 4.0f;                  // +0x618
    m_dampenSprintRotationalForceInAirMod = 1.0f;// +0x61C
    m_newCar2MinRpm = 1000.0f;                   // +0x620
    m_newCar2MaxRpm = 4000.0f;                   // +0x624
    m_newCar2UseClutchedTorque = 0;              // +0x628
    m_newCar2WheelInertia = 12.253398f;          // +0x62C
    m_newCar2EngineFeedbackMod = 0.2f;           // +0x630
    m_newCar2BrakeTorque = 2000.0f;              // +0x634
    m_newCar2EngineBrakeTorque = 0.0f;           // +0x638
    m_newCar2FrictionTorque = 100.0f;            // +0x63C
    m_newCar2FrictionMod = 1.0f;                 // +0x640
    m_newCar2SlopeTorqueMod = 1.0f;              // +0x644
    m_newCar2WheelLongFrictionMod = 1.0f;        // +0x648
    m_newCar2WheelLongDriveFrictionMod = 1.5f;   // +0x64C
    m_newCar2WheelLongPeakAt = 0.1f;             // +0x650
    m_newCar2WheelLongMinDynamicFriction = 0.7f; // +0x654
    m_newCar2WheelLongScale = 0.1f;              // +0x658
    m_newCar2WheelLatFrictionMod = 2.0f;         // +0x65C
    m_newCar2WheelLatDriveFrictionMod = 0.5f;    // +0x660
    m_newCar2WheelLatPeakAt = 0.1f;              // +0x664
    m_newCar2WheelLatMinDynamicFriction = 0.7f;  // +0x668
    m_newCar2WheelLatScale = 0.1f;               // +0x66C
    m_engineType = 0;                            // +0x590 bit 6 of it selects CarEngine
    m_propRotAxis = 2;                           // +0x5A4
    m_accumulate = false;                        // +0x5A8 byte; setAccumulate accepts a float
    m_pureRotational = false;                    // +0x5A9
    m_horizontalSpeedMagnifier = 1.0f;           // +0x5AC
    m_maxAngleOfAttack = 20.0f;                  // +0x5B0
    m_defaultAngleOfAttack = 0.0f;               // +0x5B4
    m_remoteEngineInput = io::PINone;            // +0x5BC
    m_canReverse = true;                         // +0x5EC

    // m_engineSound (+0x670): constructed by its own ctor - the ELF's last call.

    // The ctor's tail flags: physics-flags bit 1, then the eleven authored distribution keys.
    m_physicsFlags = static_cast<unsigned char>(m_physicsFlags | 2);

    m_distributionA.setKey(0, 0.7f);   m_distributionA.generateDistribution();
    m_distributionA.setKey(10, 0.8f);  m_distributionA.generateDistribution();
    m_distributionA.setKey(30, 0.9f);  m_distributionA.generateDistribution();
    m_distributionA.setKey(60, 1.0f);  m_distributionA.generateDistribution();
    m_distributionA.setKey(85, 0.85f); m_distributionA.generateDistribution();
    m_distributionA.setKey(100, 0.7f); m_distributionA.generateDistribution();
    m_distributionB.setKey(20, 3.5f);  m_distributionB.generateDistribution();
    m_distributionB.setKey(40, 2.2f);  m_distributionB.generateDistribution();
    m_distributionB.setKey(60, 1.5f);  m_distributionB.generateDistribution();
    m_distributionB.setKey(80, 1.1f);  m_distributionB.generateDistribution();
    m_distributionB.setKey(100, 0.94f);m_distributionB.generateDistribution();
}

// (D1): vptr resets + the base chain.
EngineTemplate::~EngineTemplate()
{
}

// The per-class quartet - refcount +0x218, delete on the 1->0 edge through the D0 slot.
void EngineTemplate::addRef()  { ++m_refCount; }
int  EngineTemplate::getRef()  { return m_refCount; }

int EngineTemplate::release()
{
    const int r = --m_refCount;
    if (r == 0)
        delete this;
    return r;
}

// the Bundle-chain hop plus this class's own interface at +0x214.
void* EngineTemplate::queryInterface(unsigned int iid) const
{
    return RotationalBundleTemplate::queryInterface(iid);
}

unsigned int EngineTemplate::getClassID() const { return CID_EngineTemplate; }

// a bare tail-jump to ObjectTemplate::setName. The override exists but adds nothing; the
// slot is re-emitted rather than inherited. Preserved as the forwarder it is.
void EngineTemplate::setName(const std::string& name)
{
    ObjectTemplate::setName(name);
}

// the base's initialisation, then the engine's own sound templates.
void EngineTemplate::templateInitialize()
{
    RotationalBundleTemplate::templateInitialize();
    EngineBase::initSoundTemplates(this);
}

// the standard one-CID hop into the base's chain (a literal tail-jump in the ELF).
void* EngineTemplate::queryImplementation(unsigned int classId)
{
    if (classId == CID_EngineTemplate)
        return this;
    return RotationalBundleTemplate::queryImplementation(classId);
}

// the seven accessor pairs. Four are plain store/load; three are not - see below.
void EngineTemplate::setEngineType(int v)               // , 27 B
{
    // The ELF's single unsigned comparison and the PE's two signed comparisons both accept the
    // exact inclusive range 0..0xC3. Values outside it leave the previous selector untouched.
    if (static_cast<unsigned int>(v) <= 0xC3u)
        m_engineType = v;
}
int  EngineTemplate::getEngineType() const              { return m_engineType; }
void EngineTemplate::setPropRotAxis(int v)              { m_propRotAxis = v; }
int  EngineTemplate::getPropRotAxis() const             { return m_propRotAxis; }
void EngineTemplate::setMaxAngleOfAttack(float v)       { m_maxAngleOfAttack = v; }
float EngineTemplate::getMaxAngleOfAttack() const       { return m_maxAngleOfAttack; }
void EngineTemplate::setDefaultAngleOfAttack(float v)   { m_defaultAngleOfAttack = v; }
float EngineTemplate::getDefaultAngleOfAttack() const   { return m_defaultAngleOfAttack; }
void EngineTemplate::setAccumulate(float v)             { m_accumulate = (v != 0.0f); }
float EngineTemplate::getAccumulate() const             { return m_accumulate ? 1.0f : 0.0f; }
void EngineTemplate::setPureRotational(bool v)          { m_pureRotational = v; }
bool EngineTemplate::getPureRotational() const          { return m_pureRotational != 0; }
void EngineTemplate::setHorizontalSpeedMagnifier(float v){ m_horizontalSpeedMagnifier = v; }
float EngineTemplate::getHorizontalSpeedMagnifier() const{ return m_horizontalSpeedMagnifier; }

// Engine-type bit 0x40 selects CarEngine; other types create Engine.
IObject* EngineTemplate::createObject() const
{
    if (m_engineType & 0x40)
        return new CarEngine(this);
    return new Engine(this);
}

// . Author five keys at 20/40/60/80/100,
// regenerating after each write, as in both shipped bodies. Gear count does not
// change these key positions; getGearRatio samples the resulting curve below.
void EngineTemplate::setGearRatios(float a, float b, float c, float d, float e)
{
    m_distributionB.setKey(20, a);
    m_distributionB.generateDistribution();
    m_distributionB.setKey(40, b);
    m_distributionB.generateDistribution();
    m_distributionB.setKey(60, c);
    m_distributionB.generateDistribution();
    m_distributionB.setKey(80, d);
    m_distributionB.generateDistribution();
    m_distributionB.setKey(100, e);
    m_distributionB.generateDistribution();
}

// The two curve samplers - 0x82A5EB0 (154 B) / 0x82A5F50 (142 B).
float EngineTemplate::getGearRatio(int gear) const
{
    const int n = m_numberOfGears;                                        // +0x3C8
    int g = 0;
    if (gear >= 0)
        g = gear > n ? n : gear;
    float f = static_cast<float>(g) / static_cast<float>(n);              // (n == 0 divides by zero, as shipped)
    if (f > 1.0f)
        f = 1.0f;
    f *= 100.0f;
    const int   index = static_cast<int>(f);
    const float frac  = f - static_cast<float>(index);
    return frac * m_distributionB.m_samples[index + 1] + (1.0f - frac) * m_distributionB.m_samples[index];
}

float EngineTemplate::getTorque(float rpm) const
{
    float f = std::fabs(rpm);
    if (f >= 1.0f)
        f = 1.0f;
    f *= 100.0f;
    const int   index = static_cast<int>(f);
    const float frac  = f - static_cast<float>(index);
    return (frac * m_distributionA.m_samples[index + 1] + (1.0f - frac) * m_distributionA.m_samples[index]) * m_torque;
}

// ELF 82A47A0 / dedicated PE 6093B0. Default checks intentionally differ from
// some constructor defaults; noPropellerEffectAtSpeed is emitted twice as shipped.
bool EngineTemplate::makeScript(io::IStream* stream)
{
    std::ostringstream out;
    if (!RotationalBundleTemplate::makeScript(stream)) return false;
    out << "ObjectTemplate.setEngineType ";
    // Inlined token writer: ELF 82A3C10 / PE 609140. Unknown values emit no token.
    switch (m_engineType)
    {
    case 1: out << "c_ETPlane"; break;
    case 2: out << "c_ETCar"; break;
    case 6: out << "c_ETTank"; break;
    case 9: out << "c_ETShip"; break;
    case 17: out << "c_ETRocket"; break;
    case 25: out << "c_ETTorpedo"; break;
    case 33: out << "c_ETHelicopter"; break;
    case 66: out << "c_ETNewCar"; break;
    case 70: out << "c_ETNewTank"; break;
    case 194: out << "c_ETNewCar2"; break;
    }
    out << std::endl;
    out << "ObjectTemplate.setTorque " << m_torque << std::endl;
    out << "ObjectTemplate.setDifferential " << m_differential << std::endl;
    if (m_numberOfGears != 1) out << "ObjectTemplate.setNumberOfGears " << m_numberOfGears << std::endl;
    if (m_gearUp != 0.3f) out << "ObjectTemplate.setGearUp " << m_gearUp << std::endl;
    if (m_gearDown != 0.7f) out << "ObjectTemplate.setGearDown " << m_gearDown << std::endl;
    if (m_gearChangeTime != 1.0f) out << "ObjectTemplate.setGearChangeTime " << m_gearChangeTime << std::endl;
    if (m_distributionB.m_samples[20] != 3.5f || m_distributionB.m_samples[40] != 2.2f ||
        m_distributionB.m_samples[60] != 1.5f || m_distributionB.m_samples[80] != 1.1f ||
        m_distributionB.m_samples[100] != 0.94f)
        out << "ObjectTemplate.setGearRatios " << m_distributionB.m_samples[20] << ' '
            << m_distributionB.m_samples[40] << ' ' << m_distributionB.m_samples[60] << ' '
            << m_distributionB.m_samples[80] << ' ' << m_distributionB.m_samples[100] << ' ' << std::endl;
    if (m_noPropellerEffectAtSpeed != 100.0f)
    {
        out << "ObjectTemplate.noPropellerEffectAtSpeed " << m_noPropellerEffectAtSpeed << std::endl;
        if (m_noPropellerEffectAtSpeed != 100.0f) out << "ObjectTemplate.noPropellerEffectAtSpeed " << m_noPropellerEffectAtSpeed << std::endl;
    }
    if (m_propRotAxis != 2) out << "ObjectTemplate.propRotAxis " << m_propRotAxis << std::endl;
    if (m_horizontalSpeedMagnifier != 1.0f) out << "ObjectTemplate.horizontalSpeedMagnifier " << m_horizontalSpeedMagnifier << std::endl;
    if (m_horizontalDampAngle != -1.0f) out << "ObjectTemplate.horizontalDampAngle " << m_horizontalDampAngle << std::endl;
    if (m_horizontalDampAngleFactor != 1.0f) out << "ObjectTemplate.horizontalDampAngleFactor " << m_horizontalDampAngleFactor << std::endl;
    if (m_defaultAngleOfAttack != 0.0f) out << "ObjectTemplate.defaultAngleOfAttack " << m_defaultAngleOfAttack << std::endl;
    if (m_maxAngleOfAttack != 20.0f) out << "ObjectTemplate.maxAngleOfAttack " << m_maxAngleOfAttack << std::endl;
    if (m_attackSpeed != 0.0f) out << "ObjectTemplate.attackSpeed " << m_attackSpeed << std::endl;
    if (m_accumulate) out << "ObjectTemplate.accumulate " << m_accumulate << std::endl;
    if (m_decreaseAngleToZero) out << "ObjectTemplate.decreaseAngleToZero " << m_decreaseAngleToZero << std::endl;
    if (m_decreaseAngleToZeroVerticalVel != 0.0f) out << "ObjectTemplate.decreaseAngleToZeroVerticalVel " << m_decreaseAngleToZeroVerticalVel << std::endl;
    if (m_remoteEngineInput != io::PINone) out << "ObjectTemplate.remoteEngineInput " << m_remoteEngineInput << std::endl;
    if (m_pureRotational) out << "ObjectTemplate.pureRotational " << m_pureRotational << std::endl;
    if (m_constantRpm) out << "ObjectTemplate.constantRpm " << m_constantRpm << std::endl;
    if (m_heatIncrement != -1.0f)
    {
        out << "ObjectTemplate.HeatIncrement " << m_heatIncrement << std::endl;
        if (m_coolingFactor != 0.0f) out << "ObjectTemplate.CoolingFactor " << m_coolingFactor << std::endl;
        if (m_heatBarType != 4)
            out << (m_heatBarType == 5 ? "ObjectTemplate.setHeatBarType G" : "ObjectTemplate.setHeatBarType B") << std::endl;
        if (!m_inverseHeatBar) out << "ObjectTemplate.InverseHeatBar 1" << std::endl;
    }
    if (m_spinWhenNoEngineInAir != 0.0f) out << "ObjectTemplate.spinWhenNoEngineInAir " << m_spinWhenNoEngineInAir << std::endl;
    if (m_dampHorizontalVel != 0.0f) out << "ObjectTemplate.dampHorizontalVel " << m_dampHorizontalVel << std::endl;
    if (m_dampHorizontalVelFactor != 0.0f) out << "ObjectTemplate.dampHorizontalVelFactor " << m_dampHorizontalVelFactor << std::endl;
    if (m_airFlowAffect != 0.0f) out << "ObjectTemplate.airFlowAffect " << m_airFlowAffect << std::endl;
    if (m_noEffectAtPerpSpeed != 10000.0f) out << "ObjectTemplate.noEffectAtPerpSpeed " << m_noEffectAtPerpSpeed << std::endl;
    if (!m_canReverse) out << "ObjectTemplate.canReverse 0" << std::endl;
    if (std::fabs(static_cast<double>(m_trackTurnLowAmplitude) - 2.0f) > 1.1920929e-7f) out << "ObjectTemplate.trackTurnLowAmplitude " << m_trackTurnLowAmplitude << std::endl;
    if (std::fabs(static_cast<double>(m_trackTurnLowOffset) - -1.5f) > 1.1920929e-7f) out << "ObjectTemplate.trackTurnLowOffset " << m_trackTurnLowOffset << std::endl;
    if (std::fabs(static_cast<double>(m_trackTurnHighAmplitude) - 1.3f) > 1.1920929e-7f) out << "ObjectTemplate.trackTurnHighAmplitude " << m_trackTurnHighAmplitude << std::endl;
    if (std::fabs(static_cast<double>(m_trackTurnHighOffset) - 0.45f) > 1.1920929e-7f) out << "ObjectTemplate.trackTurnHighOffset " << m_trackTurnHighOffset << std::endl;
    if (std::fabs(static_cast<double>(m_trackTurnAcceleration) - 120.0f) > 1.1920929e-7f) out << "ObjectTemplate.trackTurnAcceleration " << m_trackTurnAcceleration << std::endl;
    if (std::fabs(static_cast<double>(m_trackTurnSpeed) - 1.5f) > 1.1920929e-7f) out << "ObjectTemplate.trackTurnSpeed " << m_trackTurnSpeed << std::endl;
    if ((m_engineType & 0xC2) == 0xC2)
    {
        out << "ObjectTemplate.newCar2.minRpm " << m_newCar2MinRpm << std::endl;
        out << "ObjectTemplate.newCar2.maxRpm " << m_newCar2MaxRpm << std::endl;
        out << "ObjectTemplate.newCar2.useClutchedTorque " << (m_newCar2UseClutchedTorque != 0) << std::endl;
        out << "ObjectTemplate.newCar2.wheelInertia " << m_newCar2WheelInertia << std::endl;
        out << "ObjectTemplate.newCar2.engineFeedbackMod " << m_newCar2EngineFeedbackMod << std::endl;
        out << "ObjectTemplate.newCar2.brakeTorque " << m_newCar2BrakeTorque << std::endl;
        out << "ObjectTemplate.newCar2.engineBrakeTorque " << m_newCar2EngineBrakeTorque << std::endl;
        out << "ObjectTemplate.newCar2.frictionTorque " << m_newCar2FrictionTorque << std::endl;
        out << "ObjectTemplate.newCar2.frictionMod " << m_newCar2FrictionMod << std::endl;
        out << "ObjectTemplate.newCar2.slopeTorqueMod " << m_newCar2SlopeTorqueMod << std::endl;
        out << "ObjectTemplate.newCar2.wheelLongFrictionMod " << m_newCar2WheelLongFrictionMod << std::endl;
        out << "ObjectTemplate.newCar2.wheelLongDriveFrictionMod " << m_newCar2WheelLongDriveFrictionMod << std::endl;
        out << "ObjectTemplate.newCar2.wheelLongPeakAt " << m_newCar2WheelLongPeakAt << std::endl;
        out << "ObjectTemplate.newCar2.wheelLongMinDynamicFriction " << m_newCar2WheelLongMinDynamicFriction << std::endl;
        out << "ObjectTemplate.newCar2.wheelLongScale " << m_newCar2WheelLongScale << std::endl;
        out << "ObjectTemplate.newCar2.wheelLatFrictionMod " << m_newCar2WheelLatFrictionMod << std::endl;
        out << "ObjectTemplate.newCar2.wheelLatDriveFrictionMod " << m_newCar2WheelLatDriveFrictionMod << std::endl;
        out << "ObjectTemplate.newCar2.wheelLatPeakAt " << m_newCar2WheelLatPeakAt << std::endl;
        out << "ObjectTemplate.newCar2.wheelLatMinDynamicFriction " << m_newCar2WheelLatMinDynamicFriction << std::endl;
        out << "ObjectTemplate.newCar2.wheelLatScale " << m_newCar2WheelLatScale << std::endl;
    }
    // EngineSoundTemplate::serialize, ELF 83F2600 / PE 703980: only these two flags.
    if (m_engineSound.m_byte00) out << "ObjectTemplate.audio.newStyleAudio 1" << std::endl;
    if (m_engineSound.m_byte01) out << "ObjectTemplate.audio.transformationRelativeRoot 1" << std::endl;
    const std::string text = out.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.size())) == text.size();
}

} // namespace world
} // namespace hfe
} // namespace dice
