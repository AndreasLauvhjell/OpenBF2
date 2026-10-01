// [INFERRED] Source placement.
// [FRONTIER] Remaining methods stop by name until implemented.
// [TEMP] Temporary implementation boundaries remain.
// [CHOSEN, not recovered] Temporary implementation boundaries remain.
// [RETIRE-WHEN: aidll-01-adapters] Complete and validate the remaining AIPlayerFactory operations.

#include "AIPlayerFactory.h"

#include "AIFrontier.h"

#include "AIPlayer.h"                                 // the concrete wrapper this creates
#include "../../AIDLL/AI/AIUtils/checkPointer.h"
#include "../../../Game/GameLogic/Player.h"           // world::IPlayer::getId

#include "../../AIDLL/AI/Main/CommonSingletonServer.h"
#include "../../../Core/ClassManager.h"
#include "../SingletonServer.h"   // BFSingletonServer::updateStaticMethods

namespace dice {
namespace hfe {

extern ClassManager* classManager;

namespace ai {

// First wins in, exact match out.
IAIPlayerFactory::IAIPlayerFactory()
{
    if (!CommonSingletonServer::m_iaiPlayerFactory)
    {
        // The roster refresh. Seven interface pairs do this and nine do not - see BFTerrain.cpp.
        classManager->registerSimpleSingleton(std::string("IAIPlayerFactory"), this);
        BFSingletonServer::updateStaticMethods();
    }
}

IAIPlayerFactory::~IAIPlayerFactory()
{
    if (CommonSingletonServer::m_iaiPlayerFactory == this)
    {
        // The roster refresh. Seven interface pairs do this and nine do not - see BFTerrain.cpp.
        classManager->unregisterSimpleSingleton(std::string("IAIPlayerFactory"));
        BFSingletonServer::updateStaticMethods();
    }
}

// The ELF's _M_fill_insert / erase pair on an empty vector is `resize(256, 0)` - libstdc++ 3.x
// expands resize into exactly that branch. 256 is the player-id space, so the vector is an
// id-indexed table from the first instruction.
AIPlayerFactory::AIPlayerFactory()
{
    m_players.resize(256, 0);
}

AIPlayerFactory::~AIPlayerFactory()
{
}

// Creation overloads identify unsupported paths by their argument types in frontier diagnostics.

IAIPlayer* AIPlayerFactory::create()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPlayerFactory::create()");
}

IAIPlayer* AIPlayerFactory::create(int id)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPlayerFactory::create(int)");
}

IAIPlayer* AIPlayerFactory::create(const std::string& name)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPlayerFactory::create(const std::string&)");
}

// Wraps a player and stores it at its player id. The caller must size m_players first;
// this function neither grows the vector nor checks the index.
IAIPlayer* AIPlayerFactory::create(world::IPlayer* player)
{
    AIPlayer* const created = checkPointer(new AIPlayer(player));

    // No bounds check. See the block above.
    m_players[player->getId()] = created;

    return created;
}

void AIPlayerFactory::destroy(IAIPlayer* player)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPlayerFactory::destroy");
}

int AIPlayerFactory::getPlayerIdFromName(const std::string& name)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPlayerFactory::getPlayerIdFromName");
}

// IAIPlayerFactory's ELF slot +0x20
// (PE +0x1C) is the lookup used by BotManager::addUnspawnedBot. Both bodies reject
// negative ids and otherwise index the existing table without an upper-bound check.
// The vector's storage begins at ELF +4 / VC7.1 PE +8; use the source container.
IAIPlayer* AIPlayerFactory::getPlayerFromId(int id)
{
    if (id < 0)
        return 0;

    return m_players[id];
}

void AIPlayerFactory::setIdToPlayerMapping(IAIPlayer* player)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPlayerFactory::setIdToPlayerMapping");
}

void AIPlayerFactory::removeIdToPlayerMapping(IAIPlayer* player)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPlayerFactory::removeIdToPlayerMapping");
}

// - a bare member load, so it hands back the vector itself.
const std::vector<IAIPlayer*>& AIPlayerFactory::getAllPlayers()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPlayerFactory::getAllPlayers");
}

IAIPlayer* AIPlayerFactory::getPlayerFromLocalHumanPlayer()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPlayerFactory::getPlayerFromLocalHumanPlayer");
}

} // namespace ai
} // namespace hfe
} // namespace dice
