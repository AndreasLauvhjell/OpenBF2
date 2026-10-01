// Exposes healing, repair, ammo and radar data borrowed from the ability template.
// Keep getGLInfo before the destructor; concrete appended slots follow it.
// IObjectComponent has no virtual destructor. Source placement is [INFERRED].

#ifndef GAME_OBJECTS_GAMELOGICCOMP_IABILITYGLCOMP_H     // [INFERRED] guard style
#define GAME_OBJECTS_GAMELOGICCOMP_IABILITYGLCOMP_H

#include "../IObjectComponent.h"
#include <cstdint>

namespace dice {
namespace hfe {
namespace world {

// Template payload returned by IAbilityGLComp::getGLInfo.  PE callers read the
// three Boolean ability flags followed by the aligned radar-radius integer.
struct AbilityGLInfo
{
    bool hasHealingAbility;
    bool hasRepairingAbility;
    bool hasAmmoAbility;
    std::uint8_t padding;
    int radarRadius;
};
static_assert(sizeof(AbilityGLInfo) == 8, "retail ability payload");

class IAbilityGLComp : public IObjectComponent
{
public:
    virtual AbilityGLInfo* getGLInfo() = 0;             // +0x38 the template's payload block

    virtual ~IAbilityGLComp() {}                        // +0x3C/+0x40 - last, as in IArmorGLComp
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAMELOGICCOMP_IABILITYGLCOMP_H
