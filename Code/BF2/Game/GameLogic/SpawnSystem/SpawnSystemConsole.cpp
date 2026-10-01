// SpawnManager and spawn-point console bindings. Preserve registration order
// and the duplicate setMinSpawnHeight row. setSpawnRotation reaches the inert
// interface default. Properties support the console's set/get prefixes.
// [CHOSEN] File placement separates bindings from Core/ModuleRegistry.cpp.
#include "../../../Core/ConsoleObjectMacro.h"
#include "../../../Core/Ids.h"                            // CID_SpawnPointTemplate
#include "../../../Core/Types.h"                          // U32 - the metadata's spelling
#include "../../../Scene/Object/ObjectTemplateUtil.h"     // world::getActiveTemplate
#include "../../Objects/SpawnPointTemplate.h"
#include "SpawnManager.h"
#include <string>

namespace dice {
namespace hfe {

extern IBase* spawnManager;                              // ds:0x8AD3A00, Core/Globals.cpp

namespace {

// Stands in for the original's file-scope list (ELF 0x8AD3A08).
const char* const kSpawnSystemConsoleTag = "game.spawnSystem";

inline SpawnPointTemplate* activeSpawn()
{
    return static_cast<SpawnPointTemplate*>(world::getActiveTemplate(CID_SpawnPointTemplate));
}

void consoleListSpawnGroups()                            { static_cast<SpawnManager*>(spawnManager)->listSpawnGroups(); }
void consoleSetSpawnPositionOffset(Math::Vec3 v)         { activeSpawn()->setSpawnPositionOffset(v); }
void consoleSetSpawnRotation(Math::Vec3 v)               { activeSpawn()->setSpawnRotation(v); }   // ISpawnPointTemplate's empty body
void consoleSetSpawnPreventionDelay(float v)             { activeSpawn()->setSpawnPreventionDelay(v); }
void consoleSetSpawnAsParaTroper(bool v)                 { activeSpawn()->setSpawnAsParaTroper(v); }
void consoleSetAIEnterOnSpawn(bool v)                    { activeSpawn()->setAIEnterOnSpawn(v); }
void consoleSetEnterOnSpawn(bool v)                      { activeSpawn()->setEnterOnSpawn(v); }
void consoleSetMinSpawnHeight(float v)                   { activeSpawn()->setMinSpawnHeight(v); }
void consoleSetScatterSpawnPositions(bool v)             { activeSpawn()->setScatterSpawnPositions(v); }
void consoleSetOnlyForAI(bool v)                         { activeSpawn()->setOnlyForAI(v); }
void consoleSetOnlyForHuman(bool v)                      { activeSpawn()->setOnlyForHuman(v); }
void consoleSetAllowSpawnCloseToVehicle(bool v)          { activeSpawn()->setAllowSpawnCloseToVehicle(v); }

} // namespace

BF2_CONSOLE_OBJECT("SpawnManager", 2, 0)
BF2_CONSOLE_METHOD_0_ACTIVE(listSpawnGroups, consoleListSpawnGroups, spawnManager != 0, kSpawnSystemConsoleTag)

BF2_CONSOLE_OBJECT("ObjectTemplate", 2, 0)
BF2_CONSOLE_METHOD_1_ACTIVE(setSpawnPositionOffset, consoleSetSpawnPositionOffset,
                            world::getActiveTemplate(CID_SpawnPointTemplate) != 0, Math::Vec3, kSpawnSystemConsoleTag)
BF2_CONSOLE_METHOD_1_ACTIVE(setSpawnRotation, consoleSetSpawnRotation,
                            world::getActiveTemplate(CID_SpawnPointTemplate) != 0, Math::Vec3, kSpawnSystemConsoleTag)
BF2_CONSOLE_METHOD_1_ACTIVE(setSpawnPreventionDelay, consoleSetSpawnPreventionDelay,
                            world::getActiveTemplate(CID_SpawnPointTemplate) != 0, float, kSpawnSystemConsoleTag)
BF2_CONSOLE_PROPERTY(group, U32, activeSpawn(), setGroup, getGroup, kSpawnSystemConsoleTag)
BF2_CONSOLE_PROPERTY(ControlPointId, U32, activeSpawn(), setControlPointId, getControlPointId, kSpawnSystemConsoleTag)
BF2_CONSOLE_METHOD_1_ACTIVE(setSpawnAsParaTroper, consoleSetSpawnAsParaTroper,
                            world::getActiveTemplate(CID_SpawnPointTemplate) != 0, bool, kSpawnSystemConsoleTag)
BF2_CONSOLE_METHOD_1_ACTIVE(setAIEnterOnSpawn, consoleSetAIEnterOnSpawn,
                            world::getActiveTemplate(CID_SpawnPointTemplate) != 0, bool, kSpawnSystemConsoleTag)
BF2_CONSOLE_METHOD_1_ACTIVE(setEnterOnSpawn, consoleSetEnterOnSpawn,
                            world::getActiveTemplate(CID_SpawnPointTemplate) != 0, bool, kSpawnSystemConsoleTag)
BF2_CONSOLE_METHOD_1_ACTIVE(setMinSpawnHeight, consoleSetMinSpawnHeight,
                            world::getActiveTemplate(CID_SpawnPointTemplate) != 0, float, kSpawnSystemConsoleTag)
BF2_CONSOLE_METHOD_1_ACTIVE(setMinSpawnHeight, consoleSetMinSpawnHeight,      // the ELF's duplicate (74)
                            world::getActiveTemplate(CID_SpawnPointTemplate) != 0, float, kSpawnSystemConsoleTag)
BF2_CONSOLE_METHOD_1_ACTIVE(setScatterSpawnPositions, consoleSetScatterSpawnPositions,
                            world::getActiveTemplate(CID_SpawnPointTemplate) != 0, bool, kSpawnSystemConsoleTag)
BF2_CONSOLE_METHOD_1_ACTIVE(setOnlyForAI, consoleSetOnlyForAI,
                            world::getActiveTemplate(CID_SpawnPointTemplate) != 0, bool, kSpawnSystemConsoleTag)
BF2_CONSOLE_METHOD_1_ACTIVE(setOnlyForHuman, consoleSetOnlyForHuman,
                            world::getActiveTemplate(CID_SpawnPointTemplate) != 0, bool, kSpawnSystemConsoleTag)
BF2_CONSOLE_METHOD_1_ACTIVE(setAllowSpawnCloseToVehicle, consoleSetAllowSpawnCloseToVehicle,
                            world::getActiveTemplate(CID_SpawnPointTemplate) != 0, bool, kSpawnSystemConsoleTag)

// Handed to the console registry by SpawnSystemModule::init (Core/ModuleRegistry.cpp) beside the
// TU's 0x8AD3A08 list.
ConsoleObjectList& spawnSystemConsoleObjects()
{
    return consoleBindingList(kSpawnSystemConsoleTag);
}

} // namespace hfe
} // namespace dice
