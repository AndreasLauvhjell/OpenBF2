// Wheel/track effect settings combining SimpleObjectTemplate and EffectProperties.
// setEmitDirectionZFromSpeedDof takes bool. ELF packs it into EffectProperties tail padding;
// native builds use an ordinary member. Dedicated creation returns null; the client constructs the bundle.
// Header placement is [INFERRED].

#ifndef GAME_OBJECTS_FX_WHEELEFFECTBUNDLETEMPLATE_H     // [INFERRED] guard style
#define GAME_OBJECTS_FX_WHEELEFFECTBUNDLETEMPLATE_H

#include "EffectProperties.h"
#include "../Base/SimpleObjectTemplate.h"

namespace dice {
namespace hfe {
namespace world {

class WheelEffectBundleTemplate : public SimpleObjectTemplate, public EffectProperties
{
public:
    WheelEffectBundleTemplate();                              // 0x8216xxx, 79 B - read in full

    virtual unsigned int getClassID() const;
    virtual ~WheelEffectBundleTemplate();                     // the two bases' teardown
    virtual void templateInitialize();                        // empty, as shipped
    virtual void* queryImplementation(unsigned int classId);  // three CIDs, flat
    virtual IObject* createObject() const;                    // BF2.exe 0x6457F0 43 B; dedicated folds to NULL
    virtual bool makeScript(io::IStream*);                    // ELF/PE recovered

    virtual void setEmitDirectionZFromSpeedDof(bool);         // +0x114 bool, not float
    virtual bool getEmitDirectionZFromSpeedDof() const;       // +0x118

    unsigned char m_emitDirectionZFromSpeedDof;               // +0x2BE ctor true
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif
