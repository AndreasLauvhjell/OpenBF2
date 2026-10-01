// Engine tuning curves, vehicle physics settings and sound configuration.
// Engine-type bit 6 selects CarEngine; otherwise the factory creates Engine.
// [INFERRED] Source placement; native STL layout differs from the original layout.

#ifndef GAME_OBJECTS_ENGINETEMPLATE_H     // [INFERRED] guard style
#define GAME_OBJECTS_ENGINETEMPLATE_H

#include "RotationalBundleTemplate.h"
#include "../../Core/OverTimeDistribution.h"
#include "../../Sound/EngineSoundTemplate.h"
#include <string>

namespace dice {
namespace hfe {
class EngineSound;
class EngineNetworkable;                    // the wire reads the gear/newCar2 fields typed
namespace world {

// The RTTI-attested second base at +0x214. Its surface is the seven accessor pairs the primary
// table carries at +0x140..+0x174 [slot order attested; the interface's OWN table is not dumped].
class IEngineTemplate;    // [FRONTIER] - declared, not modelled
struct VehicleTemplateConsoleAccess;

class EngineTemplate : public RotationalBundleTemplate  /* , public IEngineTemplate @ +0x214 */
{
    friend class EngineBase;                      // the object family reads the block directly
    friend class Engine;
    friend class CarEngine;
    friend class hfe::EngineSound;                // +0x670 +0x671 and createSoundFromTemplate
    friend class PhysicsSpring;                   // +0x3CC differential, in the driven-wheel branch
    friend class ResponsePhysics;                 // +0x3CC and the newCar2 block - the wheel friction
                                                  // of its updatePhysics
    friend struct VehicleTemplateConsoleAccess;   // recovered direct-field console executors
    friend class ::dice::hfe::EngineNetworkable;  // the wire's quantiser ranges: m_newCar2Min/MaxRpm
public:
    // The two curve samplers - non-virtual, 0x82A5EB0 (154 B) / 0x82A5F50 (142 B),
    // both map their argument onto the 0..100 sample axis and lerp between the two nearest samples.
    float getGearRatio(int gear) const;           // curveB at 100 * min(clamp(gear,0,n)/n, 1)
    float getTorque(float rpm) const;             // curveA at 100 * min(|rpm|, 1), times m_torque
    void setGearRatios(float, float, float, float, float);
    EngineTemplate();                             // read in full

    // The per-class IBase quartet (refcount +0x218).
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();                      // delete on 0
    virtual void* queryInterface(unsigned int) const;
    virtual unsigned int getClassID() const;      // reads the CID global
    virtual ~EngineTemplate();
    virtual void  templateInitialize();           // base's, then EngineBase::
                                                  // initSoundTemplates
    virtual void  setName(const std::string&);    // +0x28 [FRONTIER - body unread]
    virtual void* queryImplementation(unsigned int);    // CID hop -> the base's chain
    virtual IObject* createObject() const;        // branches on engineType & 0x40: CarEngine
                                                  // (0x218) or Engine (0x220) -
    virtual bool  makeScript(io::IStream*);       // ELF/PE recovered

    // the seven accessor pairs at +0x140..+0x174, all read
    virtual void  setEngineType(int);             // accepts/stores only 0..0xC3
    virtual int   getEngineType() const;
    virtual void  setPropRotAxis(int);
    virtual int   getPropRotAxis() const;
    virtual void  setMaxAngleOfAttack(float);
    virtual float getMaxAngleOfAttack() const;
    virtual void  setDefaultAngleOfAttack(float);
    virtual float getDefaultAngleOfAttack() const;
    virtual void  setAccumulate(float);           // stores (value != 0.0f) to byte +0x5A8
    virtual float getAccumulate() const;          // integer-to-float conversion of that byte
    virtual void  setPureRotational(bool);
    virtual bool  getPureRotational() const;
    virtual void  setHorizontalSpeedMagnifier(float);
    virtual float getHorizontalSpeedMagnifier() const;

protected:
    /* 0x218 */ int  m_refCount;                  // addRef / getRef - past the +0x214 interface
                                                  // vptr, which is why it is not at the base size

    // Torque and auxiliary distributions store samples and authored-key masks.
    /* 0x21C */ OverTimeDistribution m_distributionA;   // the torque curve (getTorque samples it) - ends 0x3C7
    // Gear count and ratios affect both drivetrain calculations and the network gear-field width.
public:
    /* 0x3C8 */ int   m_numberOfGears;                  // ctor 1
    /* 0x3CC */ float m_differential;                   // ctor 10.0
    /* 0x3D0 */ float m_torque;                         // ctor 60.0
protected:
    /* 0x3D4 */ float m_gearUp;                         // ctor 0.7
    /* 0x3D8 */ float m_gearDown;                       // ctor 0.3
    /* 0x3DC */ float m_gearChangeTime;                 // ctor 1.0
    /* 0x3E0 */ OverTimeDistribution m_distributionB;   // the gear-ratio curve (getGearRatio samples it) - ends 0x58B
                                                        // NOT contiguous: +0x3C8..+0x3DF is a
                                                        // six-dword gap the ctor fills (10.0f,
                                                        // 60.0f, 0.7f, 0.3f, 1.0f and an int 1)

    // named from accessors
    /* 0x58C */ float m_noPropellerEffectAtSpeed; // ctor 500.0
    /* 0x590 */ int  m_engineType;                // get/setEngineType; .con property `engineType`
    /* 0x594 */ float m_heatIncrement;            // ctor -1.0
    /* 0x598 */ float m_coolingFactor;            // ctor 0.0
    /* 0x59C */ int   m_heatBarType;               // ctor 4
    /* 0x5A0 */ bool  m_inverseHeatBar;            // ctor false
    /* 0x5A4 */ int  m_propRotAxis;               // get/setPropRotAxis
    /* 0x5A8 */ bool m_accumulate;                // setter accepts float, stores value != 0.0f
    /* 0x5A9 */ bool m_pureRotational;            // get/setPureRotational
    /* 0x5AC */ float m_horizontalSpeedMagnifier; // = 1.0f
    /* 0x5B0 */ float m_maxAngleOfAttack;
    /* 0x5B4 */ float m_defaultAngleOfAttack;
    /* 0x5B8 */ float m_attackSpeed;               // ctor 0.0
    /* 0x5BC */ io::PlayerInputMap m_remoteEngineInput; // ctor PINone; overrides yaw input when set
    /* 0x5C0 */ bool  m_decreaseAngleToZero;       // ctor false
    /* 0x5C4 */ float m_spinWhenNoEngineInAir;     // ctor 0.0
    /* 0x5C8 */ bool  m_constantRpm;               // ctor false - EngineBase::handleMessage's forceStop test
    /* 0x5CC */ float m_horizontalDampAngle;       // ctor -1.0
    /* 0x5D0 */ float m_horizontalDampAngleFactor;// ctor 1.0
    /* 0x5D4 */ float m_dampHorizontalVel;         // ctor 0.0
    /* 0x5D8 */ float m_dampHorizontalVelFactor;   // ctor 0.0
    /* 0x5DC */ float m_decreaseAngleToZeroVerticalVel; // ctor 0.0
    /* 0x5E0 */ float m_decreaseAngleToZeroSpeed;  // ctor 0.0
    /* 0x5E4 */ float m_airFlowAffect;             // ctor 0.0
    /* 0x5E8 */ float m_noEffectAtPerpSpeed;       // ctor 10000.0
    /* 0x5EC */ bool  m_canReverse;                // ctor true; rejects input opposing acceleration
    /* 0x5F0 */ float m_trackTurnLowAmplitude;     // ctor 2.0
    /* 0x5F4 */ float m_trackTurnLowOffset;        // ctor -1.5
    /* 0x5F8 */ float m_trackTurnHighAmplitude;    // ctor 1.3
    /* 0x5FC */ float m_trackTurnHighOffset;       // ctor 0.45
    /* 0x600 */ float m_trackTurnAcceleration;     // ctor 120.0
    /* 0x604 */ float m_trackTurnSpeed;            // ctor 1.5
    /* 0x608 */ float m_sprintGear1Dampen;         // ctor 1.0
    /* 0x60C */ float m_sprintGear2Dampen;         // ctor 1.2
    /* 0x610 */ float m_sprintGear3Dampen;         // ctor 1.5
    /* 0x614 */ float m_sprintGear4Dampen;         // ctor 2.0
    /* 0x618 */ float m_sprintGear5Dampen;         // ctor 4.0
    /* 0x61C */ float m_dampenSprintRotationalForceInAirMod; // ctor 1.0
    // newCar2 console settings for RPM, braking, inertia and tire friction.
    /* 0x620 */ float m_newCar2MinRpm;                     // 1000.0
    /* 0x624 */ float m_newCar2MaxRpm;                     // 4000.0
    /* 0x628 */ unsigned char m_newCar2UseClutchedTorque;  // 0 (was m_byte628)
    /* 0x62C */ float m_newCar2WheelInertia;
    /* 0x630 */ float m_newCar2EngineFeedbackMod;          // 0.2
    /* 0x634 */ float m_newCar2BrakeTorque;                // 2000.0
    /* 0x638 */ float m_newCar2EngineBrakeTorque;          // 0
    /* 0x63C */ float m_newCar2FrictionTorque;             // 100.0
    /* 0x640 */ float m_newCar2FrictionMod;                // 1.0
    /* 0x644 */ float m_newCar2SlopeTorqueMod;             // 1.0
    /* 0x648 */ float m_newCar2WheelLongFrictionMod;       // 1.0
    /* 0x64C */ float m_newCar2WheelLongDriveFrictionMod;  // 1.5
    /* 0x650 */ float m_newCar2WheelLongPeakAt;            // 0.1
    /* 0x654 */ float m_newCar2WheelLongMinDynamicFriction;// 0.7
    /* 0x658 */ float m_newCar2WheelLongScale;             // 0.1
    /* 0x65C */ float m_newCar2WheelLatFrictionMod;        // 2.0
    /* 0x660 */ float m_newCar2WheelLatDriveFrictionMod;   // 0.5
    /* 0x664 */ float m_newCar2WheelLatPeakAt;             // 0.1
    /* 0x668 */ float m_newCar2WheelLatMinDynamicFriction; // 0.7
    /* 0x66C */ float m_newCar2WheelLatScale;              // 0.1

    // Constructor defaults are content behavior: omitted .con properties retain the tuned values.
    // Do not replace the friction, damping and gear defaults with blanket zero initialization.
    // The console-offset census and historical [UNRESOLVED] field interpretations are archived.

    // The ctor's last act constructs this in place - it is the class's tail.
    /* 0x670 */ EngineSoundTemplate m_engineSound;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_ENGINETEMPLATE_H
