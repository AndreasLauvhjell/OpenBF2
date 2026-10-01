// Exposes scripted fail/use/destroyed/repaired callouts and borrowed template message data.
// Keep the destructor after getGLInfo; concrete appended slots follow it. No playSpottedMessage slot exists.
// [INFERRED] Spotted messages are consumed through the template; that caller is unverified.
// Source placement is [INFERRED].

#ifndef GAME_OBJECTS_GAMELOGICCOMP_IRADIOGLCOMP_H     // [INFERRED] guard style
#define GAME_OBJECTS_GAMELOGICCOMP_IRADIOGLCOMP_H

#include "../IObjectComponent.h"

namespace dice {
namespace hfe {
namespace world {

class IRadioGLComp : public IObjectComponent
{
public:
    virtual void  playFailMessage(bool warn) = 0;           // +0x38
    virtual void  setFailMessageEnabled(bool enabled) = 0;  // +0x3C
    virtual void  playUseMessage(bool warn) = 0;            // +0x40
    virtual void  setUseMessageEnabled(bool enabled) = 0;   // +0x44
    virtual void  playDestroyedMessage() = 0;               // +0x48
    virtual void  playRepairedMessage() = 0;                // +0x4C
    virtual void* getGLInfo() = 0;                          // +0x50 the template's payload block

    virtual ~IRadioGLComp() {}                              // +0x54/+0x58 - last, as in IArmorGLComp
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAMELOGICCOMP_IRADIOGLCOMP_H
