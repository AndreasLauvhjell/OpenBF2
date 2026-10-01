// [INFERRED] interface header beside the attested EffectBundle.cpp implementation.
// ELF EffectBundle secondary table 0x88AA1B8 / BF2.exe 0x8FF470: three methods, no destructor.
#ifndef GAME_OBJECTS_FX_IEFFECTCONTROLLER_H
#define GAME_OBJECTS_FX_IEFFECTCONTROLLER_H

namespace dice { namespace hfe { namespace world {
class IObject;
class IEffectController
{
public:
    virtual void addNumEmittersPlaying() = 0;
    virtual void decNumEmittersPlaying() = 0;
    virtual IObject* getObject() const = 0;
};
} } }
#endif
