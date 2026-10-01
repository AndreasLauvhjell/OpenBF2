// ScoreManager console properties used by ScoreManagerSetup.con. Preserve
// registration order because active-method enumeration exposes it. Property lookup
// is case-insensitive and supports synthesized set/get prefixes.
#include "../../../Core/ConsoleObjectMacro.h"
#include "ScoreManager.h"
#include <string>

namespace dice {
namespace hfe {
namespace world {

extern IBase* scoreManager;                              // ds:0x8AD1740, Core/Globals.cpp

namespace {

// Stands in for the original's file-scope list (ELF 0x8AD1748 - eight bytes past the global above,
// the same TU-adjacency SpawnSystem and MaterialSystem show).
const char* const kScoreSystemConsoleTag = "world.scoreSystem";

// The stored IBase pointer designates ScoreManager's primary IScoreManager base.
// No adjustment is needed; static_cast would be ambiguous through its two IBase paths.
inline ScoreManager* getScoreManager()
{
    return reinterpret_cast<ScoreManager*>(scoreManager);
}

} // namespace

BF2_CONSOLE_OBJECT("ScoreManager", 2, 0)                 // fullAccess, category 0

// the seventeen counters, ConsoleClass24..40
// Every one is int, 0..1 arguments, dynamic, and dispatches through the ScoreManager vtable pair
// recorded in ScoreManager.h (+112..+244). The values on the right are what stock
// Settings/ScoreManagerSetup.con authors, i.e. what the server has been silently ignoring.
BF2_CONSOLE_PROPERTY(death,               int, getScoreManager(), setDeath,               getDeath,               kScoreSystemConsoleTag)  // 0
BF2_CONSOLE_PROPERTY(kill,                int, getScoreManager(), setKill,                getKill,                kScoreSystemConsoleTag)  // 2
BF2_CONSOLE_PROPERTY(ammo,                int, getScoreManager(), setAmmo,                getAmmo,                kScoreSystemConsoleTag)  // 1
BF2_CONSOLE_PROPERTY(assistKill,          int, getScoreManager(), setAssistKill,          getAssistKill,          kScoreSystemConsoleTag)  // 1
BF2_CONSOLE_PROPERTY(inDirectKill,        int, getScoreManager(), setInDirectKill,        getInDirectKill,        kScoreSystemConsoleTag)  // 1
BF2_CONSOLE_PROPERTY(TK,                  int, getScoreManager(), setTK,                  getTK,                  kScoreSystemConsoleTag)  // 4, written `setTk`
BF2_CONSOLE_PROPERTY(heal,                int, getScoreManager(), setHeal,                getHeal,                kScoreSystemConsoleTag)  // 1
BF2_CONSOLE_PROPERTY(soldierRevive,       int, getScoreManager(), setSoldierRevive,       getSoldierRevive,       kScoreSystemConsoleTag)  // 2
BF2_CONSOLE_PROPERTY(repair,              int, getScoreManager(), setRepair,              getRepair,              kScoreSystemConsoleTag)  // 1
BF2_CONSOLE_PROPERTY(vehicleRevive,       int, getScoreManager(), setVehicleRevive,       getVehicleRevive,       kScoreSystemConsoleTag)  // 2
BF2_CONSOLE_PROPERTY(flagCapture,         int, getScoreManager(), setFlagCapture,         getFlagCapture,         kScoreSystemConsoleTag)  // 5
BF2_CONSOLE_PROPERTY(flagReturn,          int, getScoreManager(), setFlagReturn,          getFlagReturn,          kScoreSystemConsoleTag)  // 2
BF2_CONSOLE_PROPERTY(controlPointCapture, int, getScoreManager(), setControlPointCapture, getControlPointCapture, kScoreSystemConsoleTag)  // 4
BF2_CONSOLE_PROPERTY(objective,           int, getScoreManager(), setObjective,           getObjective,           kScoreSystemConsoleTag)  // 5
BF2_CONSOLE_PROPERTY(driverPassenger,     int, getScoreManager(), setDriverPassenger,     getDriverPassenger,     kScoreSystemConsoleTag)  // 1
BF2_CONSOLE_PROPERTY(teamDamage,          int, getScoreManager(), setTeamDamage,          getTeamDamage,          kScoreSystemConsoleTag)  // 2
BF2_CONSOLE_PROPERTY(teamVehicleDamage,   int, getScoreManager(), setTeamVehicleDamage,   getTeamVehicleDamage,   kScoreSystemConsoleTag)  // 1

// the five limits, ConsoleClass45..49
// float, not int - these are the Python-settable ceilings ScoreManager.h records at +0x88..+0x98.
// The last two are percentages, which the shipped file says in a comment and nothing enforces.
BF2_CONSOLE_PROPERTY(healScoreLimit,         float, getScoreManager(), setHealScoreLimit,         getHealScoreLimit,         kScoreSystemConsoleTag)  // 100
BF2_CONSOLE_PROPERTY(repairScoreLimit,       float, getScoreManager(), setRepairScoreLimit,       getRepairScoreLimit,       kScoreSystemConsoleTag)  // 100
BF2_CONSOLE_PROPERTY(ammoScoreLimit,         float, getScoreManager(), setAmmoScoreLimit,         getAmmoScoreLimit,         kScoreSystemConsoleTag)  // 100
BF2_CONSOLE_PROPERTY(teamDamageLimit,        float, getScoreManager(), setTeamDamageLimit,        getTeamDamageLimit,        kScoreSystemConsoleTag)  // 50
BF2_CONSOLE_PROPERTY(teamVehicleDamageLimit, float, getScoreManager(), setTeamVehicleDamageLimit, getTeamVehicleDamageLimit, kScoreSystemConsoleTag)  // 25

// Handed to the console registry by ScoreSystemModule::init, the way the original registers
ConsoleObjectList& scoreSystemConsoleObjects()
{
    return consoleBindingList(kScoreSystemConsoleTag);
}

} // namespace world
} // namespace hfe
} // namespace dice
