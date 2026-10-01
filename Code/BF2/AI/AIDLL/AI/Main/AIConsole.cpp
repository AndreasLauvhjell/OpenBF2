// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining AIConsole operations.

#include "AIConsole.h"

#include "../AIBot/SensingAgentPatterns.h"     // the 4 registered patterns
#include "../AIBot/SquadManager.h"               // init's second allocation
#include "../AIUtils/checkPointer.h"             // init's two dead null checks
#include "../AIPlanInterpreter/BAPEntries.h"   // the 42 registered primitives
#include "AIDllFrontier.h"
#include "AIMain.h"                                // IAIMain::m_instance
#include "../../../../Core/ConsoleObjectMacro.h"   // the ai.init binding at the tail
#include "CommonSingletonServer.h"

namespace dice {
namespace hfe {
namespace ai {

AIConsole* AIConsole::m_instance = 0;

// Registers Bot Action Plan primitives.
// [FRONTIER] Missing primitive implementations stop by name.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.
AIConsole::AIConsole()
{
    // Publishes this console as the active singleton.
    m_instance = this;

    // the forty-two BAP primitives, in the binary's registration order
    m_bapEntries["TankMoveTo"] = new EntryTankMoveTo();
    m_bapEntries["TankMoveToObject"] = new EntryTankMoveToObject();
    m_bapEntries["TankMoveToDirection"] = new EntryTankMoveToDirection();
    m_bapEntries["TankResetControls"] = new EntryTankResetControls();
    m_bapEntries["TankTurnTo"] = new EntryTankTurnTo();
    m_bapEntries["CarMoveTo"] = new EntryCarMoveTo();
    m_bapEntries["CarMoveToDirection"] = new EntryCarMoveToDirection();
    m_bapEntries["CarMoveToObject"] = new EntryCarMoveToObject();
    m_bapEntries["CarResetControls"] = new EntryCarResetControls();
    m_bapEntries["PlaneMoveTo"] = new EntryPlaneMoveTo();
    m_bapEntries["PlaneMoveToDirection"] = new EntryPlaneMoveToDirection();
    m_bapEntries["PlaneMoveToObject"] = new EntryPlaneMoveToObject();
    m_bapEntries["PlaneAimAt"] = new EntryPlaneAimAt();
    m_bapEntries["PlaneResetControls"] = new EntryPlaneResetControls();
    m_bapEntries["BoatMoveTo"] = new EntryBoatMoveTo();
    m_bapEntries["BoatMoveToObject"] = new EntryBoatMoveToObject();
    m_bapEntries["BoatMoveToDirection"] = new EntryBoatMoveToDirection();
    m_bapEntries["BoatResetControls"] = new EntryBoatResetControls();
    m_bapEntries["InfanteryMoveTo"] = new EntryInfanteryMoveTo();
    m_bapEntries["InfanteryMoveToObject"] = new EntryInfanteryMoveToObject();
    m_bapEntries["InfanteryMoveToDirection"] = new EntryInfanteryMoveToDirection();
    m_bapEntries["InfanteryResetControls"] = new EntryInfanteryResetControls();
    m_bapEntries["Trigger"] = new EntryTrigger();
    m_bapEntries["TriggerContinously"] = new EntryTriggerContinously();
    m_bapEntries["KeyTurretAimAt"] = new EntryKeyTurretAimAt();
    m_bapEntries["MouseTurretAimAt"] = new EntryMouseTurretAimAt();
    m_bapEntries["MouseTurretLookAt"] = new EntryMouseTurretLookAt();
    m_bapEntries["Sense"] = new EntrySense();
    m_bapEntries["SoldierPose"] = new EntrySoldierPose();
    m_bapEntries["MoveToMedium"] = new EntryMoveToMedium();
    m_bapEntries["MoveToMediumBoat"] = new EntryMoveToMediumBoat();
    m_bapEntries["MoveToMediumSoldier"] = new EntryMoveToMediumSoldier();
    m_bapEntries["MoveToObjectMedium"] = new EntryMoveToObjectMedium();
    m_bapEntries["MoveToObjectMediumBoat"] = new EntryMoveToObjectMediumBoat();
    m_bapEntries["MoveToObjectMediumSoldier"] = new EntryMoveToObjectMediumSoldier();
    m_bapEntries["InfoWrapper"] = new EntryInfoWrapper();
    m_bapEntries["HeliMoveTo"] = new EntryHeliMoveTo();
    m_bapEntries["HeliMoveToDirection"] = new EntryHeliMoveToDirection();
    m_bapEntries["HeliMoveToObject"] = new EntryHeliMoveToObject();
    m_bapEntries["HeliAimAt"] = new EntryHeliAimAt();
    m_bapEntries["HeliResetControls"] = new EntryHeliResetControls();
    m_bapEntries["HeliMoveAwayFromObject"] = new EntryHeliMoveAwayFromObject();

    // the four sensing-agent patterns
    m_sensingAgentPatterns["Turret"] = new SensingAgentTurretPattern();
    m_sensingAgentPatterns["MouseFreeLook"] = new SensingAgentMouseFreeLookPattern();
    m_sensingAgentPatterns["Soldier"] = new SensingAgentSoldierPattern();
    m_sensingAgentPatterns["Null"] = new SensingAgentNullPattern();
}

// [FRONTIER] Destruction requires cleanup of owned plan entries, behavior entries and squad state.
AIConsole::~AIConsole()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::~AIConsole");
}

// [FRONTIER] Reset must delete behavior-modifier arrays and Strategy objects and return
// true, while retaining plan-entry and sensing-pattern registries. Ordered cleanup is unimplemented.
bool AIConsole::reset()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::reset");
}

// Dedicated-server console behavior: no single-player instance, no debug/camera-text
// action, and zero camera position and rotation.
AIConsole* AIConsole::getSinglePlayerInstance()
{
    return 0;
}

IAIMain* AIConsole::getActiveAIMain()
{
    return IAIMain::m_instance;
}

void AIConsole::debug()
{
}

void AIConsole::removeCameraText()
{
}

// A zeroed Vec3, by value. Three immediate zero stores into the return slot and nothing else.
Vec3 AIConsole::getRendCameraPosition()
{
    Vec3 position;
    position.x = 0.0f;
    position.y = 0.0f;
    position.z = 0.0f;
    return position;
}

Vec3 AIConsole::getRendCameraRotation()
{
    Vec3 rotation;
    rotation.x = 0.0f;
    rotation.y = 0.0f;
    rotation.z = 0.0f;
    return rotation;
}

// [FRONTIER] Remaining bot, behavior and interpreter commands require implementation.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.
BAPEntry* AIConsole::getBAPEntryFromName(std::string)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::getBAPEntryFromName");
}

ISensingAgentPattern* AIConsole::getSensingAgentPatternFromName(std::string)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::getSensingAgentPatternFromName");
}

// Initializes AI from a level script. Returns immediately when an active AI runtime already exists.
void AIConsole::init(int n)
{
    if (getActiveAIMain())
        return;

    AIMain* const aiMain = new AIMain();
    checkPointer(aiMain);

    m_aiMain = aiMain;

    // The second lookup is retail's - see above. The result is discarded.
    static_cast<AIMain*>(getActiveAIMain())->init(n);

    SquadManager* const squadManager = new SquadManager();
    checkPointer(squadManager);

    // SquadManager registers itself as a singleton; AIConsole owns it through that pointer.
    (void)squadManager;
}

void AIConsole::addBot(std::string, int, std::string, std::string)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::addBot");
}

void AIConsole::addUnspawnedBot(std::string, int, std::string)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::addUnspawnedBot");
}

void AIConsole::updateBot(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::updateBot");
}

void AIConsole::killBotName(const std::string&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::killBotName");
}

void AIConsole::killBotId(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::killBotId");
}

void AIConsole::killAllBots()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::killAllBots");
}

void* AIConsole::getBotFromId(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::getBotFromId");
}

void AIConsole::botMoveTo(float, Vec3, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::botMoveTo");
}

void AIConsole::botFlyTo(int, float, float, float, Vec3)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::botFlyTo");
}

void AIConsole::botSetWaypoints_Patrol(int, float, int, Vec3, Vec3, Vec3)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::botSetWaypoints_Patrol");
}

void AIConsole::botSetWaypoints_Patrol3d(int, float, int, Vec3, Vec3, Vec3)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::botSetWaypoints_Patrol3d");
}

void AIConsole::botGiveOrder(int, std::string, std::string)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::botGiveOrder");
}

void AIConsole::setBehaviour(int, std::string)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::setBehaviour");
}

std::string AIConsole::getBehaviour(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::getBehaviour");
}

void AIConsole::setVehicleDefaultBehaviour(std::string&, std::string&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::setVehicleDefaultBehaviour");
}

void AIConsole::setVehicleBehaviour(std::string, std::string, std::string, std::string, int,
                                    std::string, std::string)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::setVehicleBehaviour");
}

void AIConsole::setBasicBotWeights(std::string)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::setBasicBotWeights");
}

void AIConsole::createBehaviourModifiers(std::string)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::createBehaviourModifiers");
}

void AIConsole::setActiveBehaviourModifiers(std::string)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::setActiveBehaviourModifiers");
}

std::string AIConsole::getActiveBehaviourModifiers()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::getActiveBehaviourModifiers");
}

float AIConsole::getBehaviourModifier(std::string)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::getBehaviourModifier");
}

void AIConsole::setBehaviourModifier(std::string, float)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::setBehaviourModifier");
}

void AIConsole::listBehaviourModifiers()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::listBehaviourModifiers");
}

void AIConsole::addInterpreterEntry(std::string, std::string)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::addInterpreterEntry");
}

void AIConsole::addInterpreterEntry(std::string, std::string, BotLodLevel)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::addInterpreterEntry");
}

void AIConsole::removeInterpreterEntry(std::string, std::string, BotLodLevel)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::removeInterpreterEntry");
}

void AIConsole::activateDefaultInterpreter(std::string, BAPStatementType, BotLodLevel)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::activateDefaultInterpreter");
}

void AIConsole::addSAIStrategy(int, std::string)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::addSAIStrategy");
}

void AIConsole::addSensingAgentPattern(const std::string&, const std::string&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::addSensingAgentPattern");
}

void AIConsole::setSensingAgentPatternName(const std::string&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::setSensingAgentPatternName");
}

void AIConsole::setSensingAgentNullPattern(const std::string&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::setSensingAgentNullPattern");
}

void AIConsole::sensingAgentDebug()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::sensingAgentDebug");
}

void AIConsole::addAllowedUnit(const std::string&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::addAllowedUnit");
}

void AIConsole::addExpelledUnit(const std::string&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::addExpelledUnit");
}

void AIConsole::addLandingZoneUnit(const std::string&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::addLandingZoneUnit");
}

void AIConsole::setVehicleMods(const std::string&, const std::string&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::setVehicleMods");
}

void AIConsole::setStandardPersonality(const std::string&, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::setStandardPersonality");
}

float AIConsole::getFastThreshold()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::getFastThreshold");
}

void AIConsole::setFastThreshold(float)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::setFastThreshold");
}

void AIConsole::freeCameraFire()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::freeCameraFire");
}

void AIConsole::showAISettings()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::showAISettings");
}

void AIConsole::createLODManager(float, float)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::createLODManager");
}

void AIConsole::saveAll(const std::string&, const std::string&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::saveAll");
}

float AIConsole::getCurrentPitchAngle()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::getCurrentPitchAngle");
}

float AIConsole::getCurrentYawAngle()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIConsole::getCurrentYawAngle");
}

// Console binding for ai.init. Calls may reach named stops in dependent subsystems.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.

namespace {

const char* const kAiConsoleTag = "ai";

void consoleAiInit(int n)
{
    // Requires AIConsole initialization before level scripts invoke this command.
    AIConsole::m_instance->init(n);
}

} // anonymous namespace

BF2_CONSOLE_OBJECT("ai", 2, 0)
BF2_CONSOLE_METHOD_1(init, consoleAiInit, int, kAiConsoleTag)

// Handed to the console registry by AIModule::init.
ConsoleObjectList& aiConsoleObjects()
{
    return consoleBindingList(kAiConsoleTag);
}

} // namespace ai
} // namespace hfe
} // namespace dice
