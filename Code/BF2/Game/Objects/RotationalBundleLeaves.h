// Aircraft control-surface and buoyancy templates, sharing rotational sound setup.
// [INFERRED] Source placement. Physics-node attachment boundaries are documented at their implementations.

#ifndef GAME_OBJECTS_ROTATIONALBUNDLELEAVES_H     // [INFERRED] guard style
#define GAME_OBJECTS_ROTATIONALBUNDLELEAVES_H

#include "RotationalBundleTemplate.h"

namespace dice {
namespace hfe {
namespace world {

// ctor 139 B. createObject: new(0x1B8) Wing(this) [FRONTIER].
// +0x214 1.0f   +0x218 0.1f   +0x21C 0   +0x220..+0x228 a zeroed Vec3
// +0x22C byte 0 +0x230 4.91f  +0x234 1.0f  +0x238 0
class WingTemplate : public RotationalBundleTemplate
{
public:
    WingTemplate();
    virtual unsigned int getClassID() const;
    virtual ~WingTemplate();
    virtual void* queryImplementation(unsigned int);    // hop -> the base's chain
    virtual IObject* createObject() const;              // new(0x1B8) Wing
    virtual void setPhysicsNodeComponent(IObject*) const;   // +0x80 new(0x27C) PhysicsWing
    virtual bool makeScript(io::IStream*);              // ELF/PE recovered

    void setRegulateToLift(float);
    void setWingToRegulatorRatio(float);

    float         m_wingLift;             // +0x214 ctor 1.0
    float         m_flapLift;             // +0x218 ctor 0.1
    float         m_pitchOffset;          // +0x21C ctor 0
    Vec3          m_positionOffset;       // +0x220 ctor (0,0,0)
    unsigned char m_liftRegulated;        // +0x22C ctor 0 - gates the regulator block
    float         m_regulateToLift;       // +0x230 ctor 4.91
    float         m_wingToRegulatorRatio; // +0x234 ctor 1.0
    float         m_startDelay;           // +0x238 ctor 0 - in seconds; x30 gives the tick deadline
};

// ctor 114 B. createObject: new(0x1B4) FloatingBundle(this) [FRONTIER].
// +0x214 0   +0x218 1.0f   +0x21C 10.0f   +0x220 10.0f   +0x224 400.0f   +0x228 1.0f
class FloatingBundleTemplate : public RotationalBundleTemplate
{
public:
    FloatingBundleTemplate();
    virtual unsigned int getClassID() const;
    virtual ~FloatingBundleTemplate();
    virtual void* queryImplementation(unsigned int);
    virtual IObject* createObject() const;              // new(0x1B4) FloatingBundle
    virtual void setPhysicsNodeComponent(IObject*) const;   // +0x80 new(0x240) PhysicsFloatingBundle
    virtual bool makeScript(io::IStream*);              // ELF/PE recovered

    float m_waterHeight;                  // +0x214 ctor 0
    float m_hullHeight;                   // +0x218 ctor 1.0  - the submersion divisor
    float m_floatMaxLift;                 // +0x21C ctor 10.0 } lerped by the submersion factor
    float m_floatMinLift;                 // +0x220 ctor 10.0 }
    float m_dragModifier;                 // +0x224 ctor 400.0     [no recovered reader yet]
    float m_sinkingSpeedMod;              // +0x228 ctor 1.0       [no recovered reader yet]
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif
