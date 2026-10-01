// Console bindings for the selected material and material-pair cell.
// Material.active creates and selects a material; createCell selects and creates a pair.
// The damageMod availability check also creates the current pair's cell.
// Template names resolve to null when absent. The module retains its bindings on close.

#include "../../../Core/ConsoleObjectMacro.h"
#include "../../../Core/IBase.h"
#include "../../../Scene/Object/ObjectTemplateUtil.h"     // IObjectTemplate_ptrproxy
#include "../../../Physics/MaterialManager.h"
#include "../../../Physics/MMCell.h"
#include "../../../Physics/Material.h"
#include <string>

namespace dice {
namespace hfe {

namespace world { extern IBase* materialManager; }       // ds:0x8AD2820 (PE 0x861820) - Core/Globals.cpp

namespace world {

namespace {
const char* const kMaterialSystemConsoleTag = "world.materialSystem";   // stands in for ELF 0x8AD2828 / PE 0x862178

inline MaterialManager* materials() { return static_cast<MaterialManager*>(materialManager); }

// The receivers. Each is evaluated once for isObjectActive and again inside the executor, which is
// what the generated classes do (PE 0x4DAC50 / 0x4DAE9B, 0x4DC9D0 / 0x4DCA6D, 0x4DD030 / 0x4DCEE1).
inline Material* activeMaterial() { return materials()->getMaterialPtr(); }          // PE slot +0x24
inline IMMCell*  currentCellCreate() { return materials()->getCreateCell(); }         // PE slot +0x48
inline IMMCell*  currentCell() { return materials()->getCell(); }                     // PE slot +0x50

void mmCreateCell(unsigned int a, unsigned int b)
{
    materials()->getCreateCell(a, b);                                                 // PE slot +0x4C - result dropped
}
void mmAttMaterial(unsigned int id)
{
    materials()->setAttMaterial(id);                                                  // ELF +0x10 - creates + selects
}
void mmDefMaterial(unsigned int id)
{
    materials()->setDefMaterial(id);                                                  // ELF +0x40 - selects only
}
void mmSetEffectTemplate(unsigned int index, IObjectTemplate_ptrproxy t)
{
    currentCell()->setEffectTemplate(static_cast<int>(index), const_cast<IObjectTemplate*>(t.ptr));   // cell +0xC (PE)
}
void mmSetDecalTemplate(unsigned int index, IObjectTemplate_ptrproxy t)
{
    currentCell()->setDecalTemplate(static_cast<int>(index), const_cast<IObjectTemplate*>(t.ptr));    // cell +0x18 (PE)
}
void mmSetSoundTemplate(unsigned int index, IObjectTemplate_ptrproxy t)
{
    currentCell()->setSoundTemplate(static_cast<int>(index), const_cast<IObjectTemplate*>(t.ptr));    // cell +0x24 (PE)
}
} // namespace

// "Material" (PE 0x761CB0: access 2, category 0), its fifteen, in the PE's constructor order --
BF2_CONSOLE_OBJECT("Material", 2, 0)
BF2_CONSOLE_PROPERTY(active,                      unsigned int, materials(),      setAttMaterial,                 getAttMaterial,                 kMaterialSystemConsoleTag)   // ctor 0x4DA790: manager +0x10/+0x18
BF2_CONSOLE_PROPERTY(name,                        std::string,  activeMaterial(), setName,                        getName,                        kMaterialSystemConsoleTag)   // : material +0xC/+0x10
BF2_CONSOLE_PROPERTY(type,                        int,          activeMaterial(), setType,                        getType,                        kMaterialSystemConsoleTag)   // : +0x8/+0x4
BF2_CONSOLE_PROPERTY(friction,                    float,        activeMaterial(), setFriction,                    getFriction,                    kMaterialSystemConsoleTag)   // +0x14/+0x18
BF2_CONSOLE_PROPERTY(elasticity,                  float,        activeMaterial(), setElasticity,                  getElasticity,                  kMaterialSystemConsoleTag)   // +0x1C/+0x20
BF2_CONSOLE_PROPERTY(resistance,                  float,        activeMaterial(), setResistance,                  getResistance,                  kMaterialSystemConsoleTag)   // +0x24/+0x28
BF2_CONSOLE_PROPERTY(projectileCollisionHardness, float,        activeMaterial(), setProjectileCollisionHardness, getProjectileCollisionHardness, kMaterialSystemConsoleTag)   // +0x2C/+0x30
BF2_CONSOLE_PROPERTY(damageLoss,                  float,        activeMaterial(), setDamageLoss,                  getDamageLoss,                  kMaterialSystemConsoleTag)   // +0x34/+0x38
BF2_CONSOLE_PROPERTY(minDamageLoss,               float,        activeMaterial(), setMinDamageLoss,               getMinDamageLoss,               kMaterialSystemConsoleTag)   // +0x3C/+0x40
BF2_CONSOLE_PROPERTY(maxDamageLoss,               float,        activeMaterial(), setMaxDamageLoss,               getMaxDamageLoss,               kMaterialSystemConsoleTag)   // +0x44/+0x48
BF2_CONSOLE_PROPERTY(penetrationDeviation,        float,        activeMaterial(), setPenetrationDeviation,        getPenetrationDeviation,        kMaterialSystemConsoleTag)   // +0x4C/+0x50
BF2_CONSOLE_PROPERTY(overrideNeverPenetrate,      bool,         activeMaterial(), setOverrideNeverPenetrate,      getOverrideNeverPenetrate,      kMaterialSystemConsoleTag)   // +0x54/+0x58
BF2_CONSOLE_PROPERTY(hasWaterPhysics,             bool,         activeMaterial(), setHasWaterPhysics,             getHasWaterPhysics,             kMaterialSystemConsoleTag)   // +0x5C/+0x60
BF2_CONSOLE_PROPERTY(isBarbwire,                  bool,         activeMaterial(), setIsBarbwire,                  getIsBarbwire,                  kMaterialSystemConsoleTag)   // +0x64/+0x68
BF2_CONSOLE_PROPERTY(isOneSided,                  bool,         activeMaterial(), setIsOneSided,                  getIsOneSided,                  kMaterialSystemConsoleTag)   // +0x6C/+0x70
BF2_CONSOLE_PROPERTY(isSeeThrough,                bool,         activeMaterial(), setIsSeeThrough,                getIsSeeThrough,                kMaterialSystemConsoleTag)   // +0x74/+0x78

// "MaterialManager" (PE 0x761ED0: access 2, category 0) - seven in the ELF, not five
BF2_CONSOLE_OBJECT("MaterialManager", 2, 0)
// Pair selectors set attacking/defending materials for subsequent cell properties
// and template operations, without requiring createCell.
BF2_CONSOLE_METHOD_1_ACTIVE(attMaterial,       mmAttMaterial,       materials() != 0,   unsigned int,                           kMaterialSystemConsoleTag)   // ELF exec 0x8569390
BF2_CONSOLE_METHOD_1_ACTIVE(defMaterial,       mmDefMaterial,       materials() != 0,   unsigned int,                           kMaterialSystemConsoleTag)   // ELF exec 0x8569710
BF2_CONSOLE_METHOD_2_ACTIVE(createCell,        mmCreateCell,        materials() != 0,   unsigned int, unsigned int,             kMaterialSystemConsoleTag)   // ctor 0x4DC6F0, exec 0x4DC8B0
BF2_CONSOLE_PROPERTY(damageMod,                float, currentCellCreate(), setDamageMod, getDamageMod,                           kMaterialSystemConsoleTag)   //  / 0x4DCA50: cell +0x4/+0x8
BF2_CONSOLE_METHOD_2_ACTIVE(setDecalTemplate,  mmSetDecalTemplate,  currentCell() != 0, unsigned int, IObjectTemplate_ptrproxy, kMaterialSystemConsoleTag)   // exec 0x4DCCA0
BF2_CONSOLE_METHOD_2_ACTIVE(setEffectTemplate, mmSetEffectTemplate, currentCell() != 0, unsigned int, IObjectTemplate_ptrproxy, kMaterialSystemConsoleTag)   // exec 0x4DCEC0
BF2_CONSOLE_METHOD_2_ACTIVE(setSoundTemplate,  mmSetSoundTemplate,  currentCell() != 0, unsigned int, IObjectTemplate_ptrproxy, kMaterialSystemConsoleTag)   // exec 0x4DD100

// Console bindings registered by BFMaterialSystemModule. Material properties require
// an active material; manager properties require the manager. Module close leaves
// these bindings registered. [PLATFORM] Typed interfaces account for slot ordering differences.
ConsoleObjectList& materialSystemConsoleObjects()
{
    return consoleBindingList(kMaterialSystemConsoleTag);
}

} // namespace world
} // namespace hfe
} // namespace dice
