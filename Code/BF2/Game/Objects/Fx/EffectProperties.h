// Six effect properties, each controlled by six physical affectors, a base range and a used flag.
// Named virtual accessors delegate to the generic slots at the table tail; keep declaration order unchanged.
// updateVar evaluates a clamped ramp, inverted for descending ranges. Vec2 arguments/results pass by value.
// Ranges default to (1,1) except StartAge (0,0). Original ELF size is 0x15C; placement is [INFERRED].

#ifndef GAME_OBJECTS_FX_EFFECTPROPERTIES_H     // [INFERRED] guard style
#define GAME_OBJECTS_FX_EFFECTPROPERTIES_H

#include "../../../Math/Vec3.h"      // Vec2 lives beside Vec3

namespace dice {
namespace hfe {

namespace io { class IStream; }

namespace world {

// Index == the attested name-table entry (0x8988524 / 0x898853C).
enum EffectProperty
{
    EP_AlphaIntensity  = 0,      // "AlphaIntensity"
    EP_EmitScaleFactor = 1,      // "EmitScaleFactor"
    EP_EmitSpeedScale  = 2,      // "EmitSpeedScale"
    EP_AgeScaleFactor  = 3,      // "AgeScaleFactor"
    EP_StartAge        = 4,      // "StartAge"
    EP_SoundIntensity  = 5       // "SoundIntensity"
};                               // [CHOSEN enumerator spellings; the VALUES and names are attested]

enum EffectAffector
{
    EA_Speed           = 0,      // "Speed"
    EA_RotationalSpeed = 1,      // "RotationalSpeed"
    EA_Acceleration    = 2,      // "Acceleration"
    EA_Altitude        = 3,      // "Altitude"
    EA_Engine          = 4,      // "Engine"
    EA_SpeedInDof      = 5       // "SpeedInDof"
};

class EffectProperties
{
public:
    EffectProperties();                                      // disasm-verified

    virtual ~EffectProperties();                             // D1 14 B - trivial (POD members)

    // EP's OWN 1543-byte body writes the effect-property .con lines; EffectBundleTemplate
    // overrides the slot with its 1545-byte version. Both recovered from ELF/PE.
    virtual bool makeScript(io::IStream*);                   // +0x08

    // The one algorithm: ramp `value` across `range`, clamped [0,1]; a descending range (x >= y)
    // inverts the ramp. 130 B, transcribed with the ELF's exact branch structure.
    virtual float updateVar(float value, const Vec2& range); // +0x0C

    // The attested name tables, indexed raw - NO bounds check, as shipped.
    virtual const char* getEffectPropertyName(EffectProperty p);   // +0x10
    virtual const char* getEffectAffectorName(EffectAffector a);   // +0x14

    // the 96 named wrappers, slot order attested. Each virtual-dispatches a generic core.
    virtual void setAlphaIntensityRange(Vec2 v);             virtual Vec2 getAlphaIntensityRange() const;
    virtual void setAlphaIntensityUsed(bool v);              virtual bool getAlphaIntensityUsed() const;
    virtual void setAlphaIntensityFromSpeed(Vec2 v);         virtual Vec2 getAlphaIntensityFromSpeed() const;
    virtual void setAlphaIntensityFromRotationalSpeed(Vec2 v); virtual Vec2 getAlphaIntensityFromRotationalSpeed() const;
    virtual void setAlphaIntensityFromAcceleration(Vec2 v);  virtual Vec2 getAlphaIntensityFromAcceleration() const;
    virtual void setAlphaIntensityFromAltitude(Vec2 v);      virtual Vec2 getAlphaIntensityFromAltitude() const;
    virtual void setAlphaIntensityFromEngine(Vec2 v);        virtual Vec2 getAlphaIntensityFromEngine() const;
    virtual void setAlphaIntensityFromSpeedInDof(Vec2 v);    virtual Vec2 getAlphaIntensityFromSpeedInDof() const;

    virtual void setEmitScaleFactorRange(Vec2 v);            virtual Vec2 getEmitScaleFactorRange() const;
    virtual void setEmitScaleFactorUsed(bool v);             virtual bool getEmitScaleFactorUsed() const;
    virtual void setEmitScaleFactorFromSpeed(Vec2 v);        virtual Vec2 getEmitScaleFactorFromSpeed() const;
    virtual void setEmitScaleFactorFromRotationalSpeed(Vec2 v); virtual Vec2 getEmitScaleFactorFromRotationalSpeed() const;
    virtual void setEmitScaleFactorFromAcceleration(Vec2 v); virtual Vec2 getEmitScaleFactorFromAcceleration() const;
    virtual void setEmitScaleFactorFromAltitude(Vec2 v);     virtual Vec2 getEmitScaleFactorFromAltitude() const;
    virtual void setEmitScaleFactorFromEngine(Vec2 v);       virtual Vec2 getEmitScaleFactorFromEngine() const;
    virtual void setEmitScaleFactorFromSpeedInDof(Vec2 v);   virtual Vec2 getEmitScaleFactorFromSpeedInDof() const;

    virtual void setEmitSpeedScaleRange(Vec2 v);             virtual Vec2 getEmitSpeedScaleRange() const;
    virtual void setEmitSpeedScaleUsed(bool v);              virtual bool getEmitSpeedScaleUsed() const;
    virtual void setEmitSpeedScaleFromSpeed(Vec2 v);         virtual Vec2 getEmitSpeedScaleFromSpeed() const;
    virtual void setEmitSpeedScaleFromRotationalSpeed(Vec2 v); virtual Vec2 getEmitSpeedScaleFromRotationalSpeed() const;
    virtual void setEmitSpeedScaleFromAcceleration(Vec2 v);  virtual Vec2 getEmitSpeedScaleFromAcceleration() const;
    virtual void setEmitSpeedScaleFromAltitude(Vec2 v);      virtual Vec2 getEmitSpeedScaleFromAltitude() const;
    virtual void setEmitSpeedScaleFromEngine(Vec2 v);        virtual Vec2 getEmitSpeedScaleFromEngine() const;
    virtual void setEmitSpeedScaleFromSpeedInDof(Vec2 v);    virtual Vec2 getEmitSpeedScaleFromSpeedInDof() const;

    virtual void setAgeScaleFactorRange(Vec2 v);             virtual Vec2 getAgeScaleFactorRange() const;
    virtual void setAgeScaleFactorUsed(bool v);              virtual bool getAgeScaleFactorUsed() const;
    virtual void setAgeScaleFactorFromSpeed(Vec2 v);         virtual Vec2 getAgeScaleFactorFromSpeed() const;
    virtual void setAgeScaleFactorFromRotationalSpeed(Vec2 v); virtual Vec2 getAgeScaleFactorFromRotationalSpeed() const;
    virtual void setAgeScaleFactorFromAcceleration(Vec2 v);  virtual Vec2 getAgeScaleFactorFromAcceleration() const;
    virtual void setAgeScaleFactorFromAltitude(Vec2 v);      virtual Vec2 getAgeScaleFactorFromAltitude() const;
    virtual void setAgeScaleFactorFromEngine(Vec2 v);        virtual Vec2 getAgeScaleFactorFromEngine() const;
    virtual void setAgeScaleFactorFromSpeedInDof(Vec2 v);    virtual Vec2 getAgeScaleFactorFromSpeedInDof() const;

    virtual void setStartAgeRange(Vec2 v);                   virtual Vec2 getStartAgeRange() const;
    virtual void setStartAgeUsed(bool v);                    virtual bool getStartAgeUsed() const;
    virtual void setStartAgeFromSpeed(Vec2 v);               virtual Vec2 getStartAgeFromSpeed() const;
    virtual void setStartAgeFromRotationalSpeed(Vec2 v);     virtual Vec2 getStartAgeFromRotationalSpeed() const;
    virtual void setStartAgeFromAcceleration(Vec2 v);        virtual Vec2 getStartAgeFromAcceleration() const;
    virtual void setStartAgeFromAltitude(Vec2 v);            virtual Vec2 getStartAgeFromAltitude() const;
    virtual void setStartAgeFromEngine(Vec2 v);              virtual Vec2 getStartAgeFromEngine() const;
    virtual void setStartAgeFromSpeedInDof(Vec2 v);          virtual Vec2 getStartAgeFromSpeedInDof() const;

    virtual void setSoundIntensityRange(Vec2 v);             virtual Vec2 getSoundIntensityRange() const;
    virtual void setSoundIntensityUsed(bool v);              virtual bool getSoundIntensityUsed() const;
    virtual void setSoundIntensityFromSpeed(Vec2 v);         virtual Vec2 getSoundIntensityFromSpeed() const;
    virtual void setSoundIntensityFromRotationalSpeed(Vec2 v); virtual Vec2 getSoundIntensityFromRotationalSpeed() const;
    virtual void setSoundIntensityFromAcceleration(Vec2 v);  virtual Vec2 getSoundIntensityFromAcceleration() const;
    virtual void setSoundIntensityFromAltitude(Vec2 v);      virtual Vec2 getSoundIntensityFromAltitude() const;
    virtual void setSoundIntensityFromEngine(Vec2 v);        virtual Vec2 getSoundIntensityFromEngine() const;
    virtual void setSoundIntensityFromSpeedInDof(Vec2 v);    virtual Vec2 getSoundIntensityFromSpeedInDof() const;

    // the six generic cores (the table's tail; every wrapper lands here)
    virtual void setPropertyRange(EffectProperty p, Vec2 v);
    virtual Vec2 getPropertyRange(EffectProperty p) const;
    virtual void setPropertyUsed(EffectProperty p, bool used);
    virtual bool getPropertyUsed(EffectProperty p) const;
    virtual void setProperty(EffectProperty p, EffectAffector a, Vec2 v);
    virtual Vec2 getProperty(EffectProperty p, EffectAffector a) const;

    bool effectPropertiesUsed() const; // / client PE 0x67EB30, nonvirtual

    // storage (offsets within this subobject; ctor defaults noted)
    Vec2          m_range[6];                  // +0x04 all (1,1) except StartAge (0,0)
    Vec2          m_property[6][6];            // +0x34 [property][affector], zeroed
    unsigned char m_used[6];                   // +0x154 zeroed
};

// The attested name tables (0x8988524 / 0x898853C).
extern const char* const EffectPropertyNames[6];
extern const char* const EffectAffectorNames[6];

} // namespace world
} // namespace hfe
} // namespace dice

#endif
