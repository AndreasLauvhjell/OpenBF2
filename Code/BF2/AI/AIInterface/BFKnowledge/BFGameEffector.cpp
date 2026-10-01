// [INFERRED] Source placement.
// [FRONTIER] Remaining methods stop by name until implemented.
// [TEMP] Temporary implementation boundaries remain.
// [CHOSEN, not recovered] Temporary implementation boundaries remain.
// [RETIRE-WHEN: aidll-01-adapters] Complete and validate the remaining BFGameEffector operations.

#include "BFGameEffector.h"

#include "AIFrontier.h"

#include "../../AIDLL/AI/Main/CommonSingletonServer.h"
#include "../../AIDLL/AI/Main/AISettingsInterfaces.h"   // IAISettings::getBotName
#include "../../../Core/ClassManager.h"
#include "../../../Core/Ids.h"                          // IID_IGameServer
#include "../../../Game/IGame.h"                        // the `game` global
#include "../../../Game/IGameServer.h"                  // createPlayer, slot +0x20
#include "../../../Scene/Object/PlayerManager.h"        // the `playerManager` global

namespace dice {
namespace hfe {

extern ClassManager* classManager;

namespace ai {

// First wins in, exact match out.
IAIGameEffector::IAIGameEffector()
{
    if (!CommonSingletonServer::m_iaiGameEffector)
        classManager->registerSimpleSingleton(std::string("IAIGameEffector"), this);
}

IAIGameEffector::~IAIGameEffector()
{
    if (CommonSingletonServer::m_iaiGameEffector == this)
        classManager->unregisterSimpleSingleton(std::string("IAIGameEffector"));
}

BFGameEffector::BFGameEffector()
    : m_unknown04(0)
    , m_unknown08(0)
    , m_unknown0C(0)
{
}

BFGameEffector::~BFGameEffector()
{
}

int BFGameEffector::enterVehicle(IAIPlayer* player, IAIObject* vehicle)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFGameEffector::enterVehicle");
}

int BFGameEffector::exitVehicle(IAIPlayer* player)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFGameEffector::exitVehicle");
}

// Includes the overload arguments in the terminatePlayer frontier diagnostic.
int BFGameEffector::terminatePlayer(int playerId)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFGameEffector::terminatePlayer(int)");
}

int BFGameEffector::terminatePlayer(IAIPlayer* player)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFGameEffector::terminatePlayer(IAIPlayer*)");
}

// Creates an AI player through the game server and returns its player ID.
int BFGameEffector::addBot(int& a, int& b, std::string& name)
{
    if (name.empty())
    {
        const IAISettings* const settings =
            static_cast<const IAISettings*>(CommonSingletonServer::m_iaiSettings);

        name.assign(settings->getBotName());
    }

    // equal weights - the engine balances, not the AI.
    b = world::playerManager->getTeamToAddPlayer(1.0f, 1.0f);

    IGameServer* const server =
        game ? static_cast<IGameServer*>(game->queryInterface(IID_IGameServer)) : 0;

    if (server)
    {
        // asked A second time. See the block above; the binary does not reuse `server`.
        IGameServer* const target =
            game ? static_cast<IGameServer*>(game->queryInterface(IID_IGameServer)) : 0;

        a = target->createPlayer(name, true);
        return a;
    }

    a = -1;
    return 0;
}

int BFGameEffector::respawnBot(IAIPlayer* player)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFGameEffector::respawnBot");
}

void BFGameEffector::forceSpawnBot(IAIPlayer* player, const Vec3& position, const Vec3& rotation)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFGameEffector::forceSpawnBot");
}

int BFGameEffector::giveDamage(IAIObject* target, world::IPlayer* from, int type, float amount)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFGameEffector::giveDamage");
}

int BFGameEffector::playEffect(const Vec3& position, int a, int b)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFGameEffector::playEffect");
}

// 5 bytes - A bare ret. dice's own empty body, not a gap.
void BFGameEffector::playSoundEffects(double dt)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFGameEffector::playSoundEffects");
}

int BFGameEffector::suicide(IAIPlayer* player)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFGameEffector::suicide");
}

} // namespace ai
} // namespace hfe
} // namespace dice
