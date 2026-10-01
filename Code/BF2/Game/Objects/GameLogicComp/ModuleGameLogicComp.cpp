// Registers ObjectTemplate armor, ability and radio component bindings.
// Bindings obtain the active template's component and trust the preceding isActive gate.
// Properties accept zero arguments for get and one for set; armor-effect methods require four arguments.
// Preserve component declaration/slot order and ObjectTemplate access/category defaults.
// Source path is attested; module lifecycle bodies remain in Core/ModuleRegistry.cpp.

#include "../../../Core/ConsoleObjectMacro.h"
#include "../../../Core/Ids.h"                            // IID_I{Armor,Ability,Radio}GLCompTemplate
#include "../../../Scene/Object/ObjectTemplateUtil.h"     // world::getActiveTemplateComponent
#include "../../../Math/Vec3.h"
#include "ArmorGLCompTemplate.h"
#include "../AbilityGLCompTemplate.h"
#include "../RadioGLCompTemplate.h"
#include <string>

namespace dice {
namespace hfe {
namespace world {

namespace {
const char* const kGameLogicCompConsoleTag = "world.gameLogicComp";     // stands in for ELF 0x8B0A564 / PE 0x87EFA0

// The receiver of every armor binding: the active template's armor component. A null answers
// isObjectActive; the executor then trusts it.
ArmorGLCompTemplate* activeArmor()
{
    return static_cast<ArmorGLCompTemplate*>(getActiveTemplateComponent(IID_IArmorGLCompTemplate));   // 50347
}
AbilityGLCompTemplate* activeAbility()
{
    return static_cast<AbilityGLCompTemplate*>(getActiveTemplateComponent(IID_IAbilityGLCompTemplate));   // 50355
}
RadioGLCompTemplate* activeRadio()
{
    return static_cast<RadioGLCompTemplate*>(getActiveTemplateComponent(IID_IRadioGLCompTemplate));   // 50359
}

void armorAddArmorEffect(int state, const std::string& name, const Vec3& a, const Vec3& b)
{
    activeArmor()->addArmorEffect(state, name, a, b);                                           // slot +0x124
}
void armorAddArmorEffectSpectacular(int state, const std::string& name, const Vec3& a, const Vec3& b)
{
    activeArmor()->addArmorEffectSpectacular(state, name, a, b);                                // slot +0x128
}
void armorAddWreckArmorEffect(int state, const std::string& name, const Vec3& a, const Vec3& b)
{
    activeArmor()->addWreckArmorEffect(state, name, a, b);                                      // slot +0x12C
}
} // namespace

// The TU's defaults (PE 0x76720E): object "ObjectTemplate", access 2, category 0.
BF2_CONSOLE_OBJECT("ObjectTemplate", 2, 0)

// the 38 properties, in the PE's executor address order (ctor 0x528E70 first)
// name                              T      receiver       setter                                getter                                slots (set/get)
BF2_CONSOLE_PROPERTY(armor.hitPoints,                    float, activeArmor(), setHitPoints,                   getHitPoints,                   kGameLogicCompConsoleTag)   // +0x34/+0x38
BF2_CONSOLE_PROPERTY(armor.maxHitPoints,                 float, activeArmor(), setMaxHitPoints,                getMaxHitPoints,                kGameLogicCompConsoleTag)   // +0x3C/+0x40
BF2_CONSOLE_PROPERTY(armor.wreckHitPoints,               float, activeArmor(), setWreckHitPoints,              getWreckHitPoints,              kGameLogicCompConsoleTag)   // +0x2C/+0x30
BF2_CONSOLE_PROPERTY(armor.attackDamage,                 float, activeArmor(), setAttackDamage,                getAttackDamage,                kGameLogicCompConsoleTag)   // +0x44/+0x48
BF2_CONSOLE_PROPERTY(armor.angleMod,                     float, activeArmor(), setAngleMod,                    getAngleMod,                    kGameLogicCompConsoleTag)   // +0x4C/+0x50
BF2_CONSOLE_PROPERTY(armor.speedMod,                     float, activeArmor(), setSpeedMod,                    getSpeedMod,                    kGameLogicCompConsoleTag)   // +0x54/+0x58
BF2_CONSOLE_PROPERTY(armor.defaultMaterial,              int,   activeArmor(), setDefaultMaterial,             getDefaultMaterial,             kGameLogicCompConsoleTag)   // +0x130/+0x134
BF2_CONSOLE_PROPERTY(armor.explosionRadius,              float, activeArmor(), setExplosionRadius,             getExplosionRadius,             kGameLogicCompConsoleTag)   // +0x5C/+0x60
BF2_CONSOLE_PROPERTY(armor.explosionDamage,              float, activeArmor(), setExplosionDamage,             getExplosionDamage,             kGameLogicCompConsoleTag)   // +0x64/+0x68
BF2_CONSOLE_PROPERTY(armor.explosionMaterial,            int,   activeArmor(), setExplosionMaterial,           getExplosionMaterial,           kGameLogicCompConsoleTag)   // +0x6C/+0x70
BF2_CONSOLE_PROPERTY(armor.explosionForce,               float, activeArmor(), setExplosionForce,              getExplosionForce,              kGameLogicCompConsoleTag)   // +0x74/+0x78
BF2_CONSOLE_PROPERTY(armor.explosionForceMax,            float, activeArmor(), setExplosionForceMax,           getExplosionForceMax,           kGameLogicCompConsoleTag)   // +0x7C/+0x80
BF2_CONSOLE_PROPERTY(armor.explosionForceMod,            float, activeArmor(), setExplosionForceMod,           getExplosionForceMod,           kGameLogicCompConsoleTag)   // +0x84/+0x88
BF2_CONSOLE_PROPERTY(armor.wreckExplosionRadius,         float, activeArmor(), setWreckExplosionRadius,        getWreckExplosionRadius,        kGameLogicCompConsoleTag)   // +0x8C/+0x90
BF2_CONSOLE_PROPERTY(armor.wreckExplosionDamage,         float, activeArmor(), setWreckExplosionDamage,        getWreckExplosionDamage,        kGameLogicCompConsoleTag)   // +0x94/+0x98
BF2_CONSOLE_PROPERTY(armor.wreckExplosionMaterial,       int,   activeArmor(), setWreckExplosionMaterial,      getWreckExplosionMaterial,      kGameLogicCompConsoleTag)   // +0x9C/+0xA0
BF2_CONSOLE_PROPERTY(armor.wreckExplosionForce,          float, activeArmor(), setWreckExplosionForce,         getWreckExplosionForce,         kGameLogicCompConsoleTag)   // +0xA4/+0xA8
BF2_CONSOLE_PROPERTY(armor.wreckExplosionForceMax,       float, activeArmor(), setWreckExplosionForceMax,      getWreckExplosionForceMax,      kGameLogicCompConsoleTag)   // +0xAC/+0xB0
BF2_CONSOLE_PROPERTY(armor.wreckExplosionForceMod,       float, activeArmor(), setWreckExplosionForceMod,      getWreckExplosionForceMod,      kGameLogicCompConsoleTag)   // +0xB4/+0xB8
BF2_CONSOLE_PROPERTY(armor.hpLostWhileCriticalDamage,    float, activeArmor(), setHpLostWhileCriticalDamage,   getHpLostWhileCriticalDamage,   kGameLogicCompConsoleTag)   // +0xBC/+0xC0
BF2_CONSOLE_PROPERTY(armor.hpLostWhileUpSideDown,        float, activeArmor(), setHpLostWhileUpSideDown,       getHpLostWhileUpSideDown,       kGameLogicCompConsoleTag)   // +0xC4/+0xC8
BF2_CONSOLE_PROPERTY(armor.hpLostWhileInWater,           float, activeArmor(), setHpLostWhileInWater,          getHpLostWhileInWater,          kGameLogicCompConsoleTag)   // +0xCC/+0xD0
BF2_CONSOLE_PROPERTY(armor.hpLostWhileInDeepWater,       float, activeArmor(), setHpLostWhileInDeepWater,      getHpLostWhileInDeepWater,      kGameLogicCompConsoleTag)   // +0xD4/+0xD8
BF2_CONSOLE_PROPERTY(armor.waterDamageDelay,             float, activeArmor(), setWaterDamageDelay,            getWaterDamageDelay,            kGameLogicCompConsoleTag)   // +0xDC/+0xE0
BF2_CONSOLE_PROPERTY(armor.deepWaterDamageDelay,         float, activeArmor(), setDeepWaterDamageDelay,        getDeepWaterDamageDelay,        kGameLogicCompConsoleTag)   // +0xE4/+0xE8
BF2_CONSOLE_PROPERTY(armor.waterLevel,                   float, activeArmor(), setWaterLevel,                  getWaterLevel,                  kGameLogicCompConsoleTag)   // +0xEC/+0xF0
BF2_CONSOLE_PROPERTY(armor.deepWaterLevel,               float, activeArmor(), setDeepWaterLevel,              getDeepWaterLevel,              kGameLogicCompConsoleTag)   // +0xF4/+0xF8
BF2_CONSOLE_PROPERTY(armor.canBeRepairedWhenWreck,       bool,  activeArmor(), setCanBeRepairedWhenWreck,      getCanBeRepairedWhenWreck,      kGameLogicCompConsoleTag)   // +0xFC/+0x100
BF2_CONSOLE_PROPERTY(armor.criticalDamage,               float, activeArmor(), setCriticalDamage,              getCriticalDamage,              kGameLogicCompConsoleTag)   // +0x104/+0x108
BF2_CONSOLE_PROPERTY(armor.timeToStayAsWreck,            float, activeArmor(), setTimeToStayAsWreck,           getTimeToStayAsWreck,           kGameLogicCompConsoleTag)   // +0x10C/+0x110
BF2_CONSOLE_PROPERTY(armor.timeToStayAfterDestroyed,     float, activeArmor(), setTimeToStayAfterDestroyed,    getTimeToStayAfterDestroyed,    kGameLogicCompConsoleTag)   // +0x114/+0x118
BF2_CONSOLE_PROPERTY(armor.alignLastEffectToHitDirection,bool,  activeArmor(), setAlignLastEffectToHitDirection, getAlignLastEffectToHitDirection, kGameLogicCompConsoleTag) // +0x11C/+0x120
BF2_CONSOLE_PROPERTY(armor.wreckDelay,                   float, activeArmor(), setWreckDelay,                  getWreckDelay,                  kGameLogicCompConsoleTag)   // +0x138/+0x13C
BF2_CONSOLE_PROPERTY(armor.hideChildrenOnSpectacularDeath, bool, activeArmor(), setHideChildrenOnSpectacularDeath, getHideChildrenOnSpectacularDeath, kGameLogicCompConsoleTag) // +0x140/+0x144
BF2_CONSOLE_PROPERTY(armor.destroyOnSpectacularDeath,    bool,  activeArmor(), setDestroyOnSpectacularDeath,   getDestroyOnSpectacularDeath,   kGameLogicCompConsoleTag)   // +0x148/+0x14C
BF2_CONSOLE_PROPERTY(armor.canBeDestroyed,               bool,  activeArmor(), setCanBeDestroyed,              getCanBeDestroyed,              kGameLogicCompConsoleTag)   // +0x150/+0x154
BF2_CONSOLE_PROPERTY(armor.showDamageAsDirt,             float, activeArmor(), setShowDamageAsDirt,            getShowDamageAsDirt,            kGameLogicCompConsoleTag)   // +0x158/+0x15C
BF2_CONSOLE_PROPERTY(armor.useMMOnDeath,                 bool,  activeArmor(), setUseMMOnDeath,                getUseMMOnDeath,                kGameLogicCompConsoleTag)   // +0x160/+0x164

// the three effect methods (ctors 0x52CF00 / 0x52D300 / 0x52D700; executors 0x52D0B0 / 0x52D4B0 /
// ): argc exactly 4 - the damage state, the effect template's name, two vectors
BF2_CONSOLE_METHOD_4(armor.addArmorEffect,            armorAddArmorEffect,            activeArmor() != 0, int, std::string, Vec3, Vec3, kGameLogicCompConsoleTag)   // +0x124
BF2_CONSOLE_METHOD_4(armor.addArmorEffectSpectacular, armorAddArmorEffectSpectacular, activeArmor() != 0, int, std::string, Vec3, Vec3, kGameLogicCompConsoleTag)   // +0x128
BF2_CONSOLE_METHOD_4(armor.addWreckArmorEffect,       armorAddWreckArmorEffect,       activeArmor() != 0, int, std::string, Vec3, Vec3, kGameLogicCompConsoleTag)   // +0x12C

// the four ability properties (ELF construction order; slots from vtable 0x88AED00)
BF2_CONSOLE_PROPERTY(ability.hasHealingAbility,          bool,        activeAbility(), setHasHealingAbility,   getHasHealingAbility,   kGameLogicCompConsoleTag)   // +0x2C/+0x30
BF2_CONSOLE_PROPERTY(ability.hasRepairingAbility,        bool,        activeAbility(), setHasRepairingAbility, getHasRepairingAbility, kGameLogicCompConsoleTag)   // +0x34/+0x38
BF2_CONSOLE_PROPERTY(ability.hasAmmoAbility,             bool,        activeAbility(), setHasAmmoAbility,      getHasAmmoAbility,      kGameLogicCompConsoleTag)   // +0x3C/+0x40
BF2_CONSOLE_PROPERTY(ability.radarRadius,                int,         activeAbility(), setRadarRadius,         getRadarRadius,         kGameLogicCompConsoleTag)   // +0x44/+0x48

// the five radio properties (ELF construction order; slots from vtable 0x88B0040). The stock
// tweaks spell the group `Radio.*` capitalised - the console multimap is case-insensitive
BF2_CONSOLE_PROPERTY(radio.failMessage,                  std::string, activeRadio(),   setFailMessage,         getFailMessage,         kGameLogicCompConsoleTag)   // +0x2C/+0x30
BF2_CONSOLE_PROPERTY(radio.useMessage,                   std::string, activeRadio(),   setUseMessage,          getUseMessage,          kGameLogicCompConsoleTag)   // +0x34/+0x38
BF2_CONSOLE_PROPERTY(radio.destroyedMessage,             std::string, activeRadio(),   setDestroyedMessage,    getDestroyedMessage,    kGameLogicCompConsoleTag)   // +0x3C/+0x40
BF2_CONSOLE_PROPERTY(radio.repairedMessage,              std::string, activeRadio(),   setRepairedMessage,     getRepairedMessage,     kGameLogicCompConsoleTag)   // +0x44/+0x48
BF2_CONSOLE_PROPERTY(radio.spottedMessage,               std::string, activeRadio(),   setSpottedMessage,      getSpottedMessage,      kGameLogicCompConsoleTag)   // +0x4C/+0x50

// Handed to the console registry by GameLogicCompModule::init (Core/ModuleRegistry.cpp), the way
// the original registers PE 0x87EFA0 / ELF 0x8B0A564 - the 37-byte init shape at PE 0x528A20.
ConsoleObjectList& gameLogicCompConsoleObjects()
{
    return consoleBindingList(kGameLogicCompConsoleTag);
}

} // namespace world
} // namespace hfe
} // namespace dice
