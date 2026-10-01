// [INFERRED] path; interface identity is ELF RTTI-attested.
// Complete ELF88B2008 and PE90BA20 tables,
#ifndef GAME_OBJECTS_IWEAPONHUDCOMP_H
#define GAME_OBJECTS_IWEAPONHUDCOMP_H
#include "IObjectComponent.h"
#include <string>
namespace dice { namespace hfe { namespace world {
enum WarningState : int; // ELF setLockState mangling; values remain unnamed.
struct WeaponHudIcons;
struct WeaponHudInfo; // [INFERRED] record name; complete storage is in WeaponHudCompTemplate.h.
class IWeaponHudComp : public IObjectComponent {
public:
    virtual int getGuiIndex(bool) = 0;               // +38 ELF822E620
    virtual std::string getCrossHairIcon() = 0;      // +3C ELF822E7D0, hidden sret
    virtual void setLockState(WarningState) = 0;     // +40 ELF822E960 / PE68A6F0
    virtual void playOverheatSound() = 0;            // +44 ELF822EA40
    virtual bool getShowOnCamMode(int) = 0;          // +48 ELF822E8A0, eax0/1
    virtual WeaponHudIcons* getHudIcons() = 0;       // +4C ELF822E600, template+60
    virtual WeaponHudInfo* getHudInfo() = 0;         // +50 ELF822E610, template+18
    virtual ~IWeaponHudComp() {}                     // +54 PE / +54,+58 ELF
    virtual WarningState getLockState() = 0;         // +58 PE / +5C ELF822EBD0
};
} } }
#endif
