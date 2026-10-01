// Aircraft landing-gear thresholds, delays and inversion setting.
// Inherits rotational sound setup from RotationalBundleTemplate. [INFERRED] Source placement.

#ifndef GAME_OBJECTS_LANDINGGEARTEMPLATE_H     // [INFERRED] guard style
#define GAME_OBJECTS_LANDINGGEARTEMPLATE_H

#include "RotationalBundleTemplate.h"

namespace dice {
namespace hfe {
namespace world {

class LandingGearTemplate : public RotationalBundleTemplate
{
public:
    LandingGearTemplate();                              // read in full

    virtual unsigned int getClassID() const;            // reads the CID global
    virtual ~LandingGearTemplate();
    virtual void* queryImplementation(unsigned int);    // CID hop -> the base's chain
    virtual IObject* createObject() const;              // new(0x1CC) LandingGear(this)
    virtual bool makeScript(io::IStream*);              // ELF/PE recovered

    // . Zero-initialised in retail; its generated console executor is at 0x87A2890.
    static bool m_forceGearUp;

    /* 0x214 */ float m_gearDownHeight;                 // ctor 30.0f
    /* 0x218 */ float m_gearUpHeight;                   // ctor 20.0f
    /* 0x21C */ float m_gearDownSpeed;                  // ctor 60.0f
    /* 0x220 */ float m_gearUpSpeed;                    // ctor 70.0f
    /* 0x224 */ float m_gearUpDelay;                    // ctor 0.0f
    /* 0x228 */ float m_gearDownDelay;                  // ctor 0.0f
    /* 0x22C */ bool  m_invertGear;                     // ctor false
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif
