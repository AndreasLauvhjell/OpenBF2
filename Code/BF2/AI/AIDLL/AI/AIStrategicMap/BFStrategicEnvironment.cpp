// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining BFStrategicEnvironment operations.

#include "BFStrategicEnvironment.h"

#include <string>

#include "../../../../Core/ClassManager.h"
#include "../Main/CommonSingletonServer.h"
#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {

extern ClassManager* classManager;

namespace ai {

// IAIStrategicEnvironment
IAIStrategicEnvironment::IAIStrategicEnvironment()
{
    // the guard reads the roster static, not the registry. Reproduced.
    if (!AISingletonServer::m_iaiStrategicEnvironment)
        classManager->registerSimpleSingleton("IAIStrategicEnvironment", this);
}

IAIStrategicEnvironment::~IAIStrategicEnvironment()
{
    if (AISingletonServer::m_iaiStrategicEnvironment == this)
        classManager->unregisterSimpleSingleton("IAIStrategicEnvironment");
}

// Stateless adapter; construction requires only the base-class initialization.
BFStrategicEnvironment::BFStrategicEnvironment()
{
}

// The vptr reset and the base call, which again is what C++ emits.
BFStrategicEnvironment::~BFStrategicEnvironment()
{
}

int BFStrategicEnvironment::getEnemyStrengths(Bot *)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFStrategicEnvironment::getEnemyStrengths");
}

int BFStrategicEnvironment::getEnemyTypes(Bot *)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFStrategicEnvironment::getEnemyTypes");
}

int BFStrategicEnvironment::getTemperature(Bot *)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFStrategicEnvironment::getTemperature");
}

float BFStrategicEnvironment::getTemperature(int, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFStrategicEnvironment::getTemperature");
}

int BFStrategicEnvironment::getSecurity(Bot *)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFStrategicEnvironment::getSecurity");
}

float BFStrategicEnvironment::getSecurity(int, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFStrategicEnvironment::getSecurity");
}

int BFStrategicEnvironment::getControl(Bot *)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFStrategicEnvironment::getControl");
}

float BFStrategicEnvironment::getControl(int, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFStrategicEnvironment::getControl");
}

int BFStrategicEnvironment::getFriendlyBattleStr(Bot *)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFStrategicEnvironment::getFriendlyBattleStr");
}

float BFStrategicEnvironment::getFriendlyBattleStr(int, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFStrategicEnvironment::getFriendlyBattleStr");
}

int BFStrategicEnvironment::getHostileBattleStr(Bot *)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFStrategicEnvironment::getHostileBattleStr");
}

float BFStrategicEnvironment::getHostileBattleStr(int, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFStrategicEnvironment::getHostileBattleStr");
}

} // namespace ai
} // namespace hfe
} // namespace dice
