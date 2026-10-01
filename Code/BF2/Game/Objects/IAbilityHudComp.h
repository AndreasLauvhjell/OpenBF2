// [INFERRED] path and opaque info-block return spelling. Exact method names,
// bool parameters and constness: ELF AbilityHudComp table88B01A8, RTTI88B0244.
// PE table90BAA0 matches every slot through +60.
#ifndef GAME_OBJECTS_IABILITYHUDCOMP_H
#define GAME_OBJECTS_IABILITYHUDCOMP_H
#include "IObjectComponent.h"
namespace dice { namespace hfe { namespace world {
struct AbilityHudInfo; // [INFERRED] opaque block at template+10, beginning with three strings
class IAbilityHudComp : public IObjectComponent {
public:
    virtual AbilityHudInfo* getHudInfo() = 0; // +38 ELF8224D80 / PE68AD60; address return
    virtual void setHealingActive(bool) = 0; // +3C ELF8224D90 / PE68A8D0
    virtual void setRepairingActive(bool) = 0; // +40 ELF8224E00 / PE68A920
    virtual void setAmmoActive(bool) = 0; // +44 ELF8224E70 / PE68A970
    virtual void setGetHealActive(bool) = 0; // +48 ELF8224EE0 / PE68A9C0
    virtual void setGetRepairActive(bool) = 0; // +4C ELF8224F50 / PE68AA10
    virtual void setGetAmmoActive(bool) = 0; // +50 ELF8224FC0 / PE68AA60
    virtual bool isPlayingHealSound() const = 0; // +54 ELF8225200 / PE68A8A0
    virtual bool isPlayingRepairSound() const = 0; // +58 ELF8225210 / PE68A8B0
    virtual bool isPlayingAmmoSound() const = 0; // +5C ELF8225220 / PE68A8C0
    virtual void forceStopAllSound() = 0; // +60 ELF8225030 / PE68AAB0
protected:
    // The concrete AbilityHudComp appends its destructor at +64 (ELF pair64/68).
    // A concrete table alone does not prove a virtual destructor on this interface.
    ~IAbilityHudComp() = default;
};
} } }
#endif
