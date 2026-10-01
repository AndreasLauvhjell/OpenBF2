// [INFERRED] Source placement.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining SquadManager operations.

#include "SquadManager.h"

#include "SquadCoordinator.h"                        // the constructor builds two per squad
#include "BotMain.h"
#include "BotSquad.h"
#include "../Main/AIDllFrontier.h"
#include "../Main/IBotManager.h"
#include "../Main/AIMain.h"
#include "../Main/CommonSingletonServer.h"
#include "../AIStrategic/SAI.h"
#include "../../../AIInterface/BFKnowledge/BFGame.h" // IAIGame::getMaxNumberOfSquads
#include "../../../AIInterface/BFKnowledge/AIPlayerFactory.h"
#include "../../../AIInterface/BFKnowledge/AIPlayer.h"
#include "../../../AIInterface/BFKnowledge/BFSquad.h"
#include "../../../AIInterface/BFKnowledge/BFCommander.h"
#include "../../../AIInterface/BFKnowledge/BFTimer.h"
#include "../../../../Core/SettingsRepostitory.h"
#include "../../../../Core/StringUtil.h"

namespace dice {
namespace hfe {
namespace ai {

// Both materialize the full integer sum.
// Side is one-based. Preserve the fresh vector bound and indexed receiver at every call.
int SquadManager::getSquadStrengths(int side, int areaIndex, StrategicStrengthType type)
{
    int total = 0;
    for (std::size_t index = 0; index < m_coordinators[side - 1].size(); ++index)
    {
        if (m_coordinators[side - 1][index]->getMissionStatus() == 2 &&
            m_coordinators[side - 1][index]->isSquadAssignedToArea(areaIndex))
        {
            total += m_coordinators[side - 1][index]->getStrategicStrength(type);
        }
    }
    return total;
}

SquadManager* SquadManager::m_instance = 0;

// One coordinator per call, alternating
// sides. Re-read both cursors and side-zero's size after the coordinator callback; no empty
// index or null guard exists. The size-minus-one comparison is unsigned, including underflow.
void SquadManager::update(double time)
{
    updateArtilleryStatus(time);
    updateCommanderMessages();
    m_sideCursor = m_sideCursor == 0;
    m_coordinators[m_sideCursor][m_squadCursor]->update(time);
    if (m_sideCursor == 1)
    {
        if (static_cast<unsigned int>(m_squadCursor) < m_coordinators[0].size() - 1)
            m_squadCursor = static_cast<int>(static_cast<unsigned int>(m_squadCursor) + 1);
        else
        {
            m_squadCursor = 0;
            updateHumanPlayers(time);
        }
    }
}

// Strict ordered expiry, then clear the flag before any
// path callback. Each expiry visits both sides, using live vector bounds/entries. Side one's
// flag, timestamp and duration are read after side zero's callbacks; simultaneous expiry
// therefore runs two complete traversals.
void SquadManager::updateArtilleryStatus(double time)
{
    SideBlock* blocks[2] = { &m_sideBlock0, &m_sideBlock1 };
    for (unsigned int block = 0; block < 2; ++block)
    {
        if (blocks[block]->m_b0 && time > m_double156 + blocks[block]->m_artilleryStartTime)
        {
            blocks[block]->m_b0 = false;
            for (unsigned int side = 0; side < 2; ++side)
                for (unsigned int index = 0; index < m_coordinators[side].size(); ++index)
                    m_coordinators[side][index]->updateSquadPaths();
        }
    }
}

// Purge messages for each human commander before reading
// game time once. Dispatch only types 0/1, but delete every expired type. The queued pointer
// is fetched again after the send callback for deletion, and the list end remains live.
void SquadManager::updateCommanderMessages()
{
    for (int side = 0; side < 2; ++side)
    {
        if (IAIMain::m_instance->getSAI(side + 1)->isPlayerCommander())
        {
            for (std::list<CommanderMessage*>::iterator i = m_commanderMessages.begin();
                 i != m_commanderMessages.end(); )
            {
                if ((*i)->m_sideIndex == side)
                {
                    delete *i;
                    m_commanderMessages.erase(i++);
                }
                else
                    ++i;
            }
        }
    }
    const double now = static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
    for (std::list<CommanderMessage*>::iterator i = m_commanderMessages.begin();
         i != m_commanderMessages.end(); )
    {
        if (now > (*i)->m_dueTime)
        {
            switch (static_cast<int>((*i)->m_type))
            {
            case 0:
                static_cast<IBFCommander*>(CommonSingletonServer::m_iBFCommander)
                    ->sendAttackMessage((*i)->m_sideIndex + 1, (*i)->m_squadIndex + 1);
                break;
            case 1:
                static_cast<IBFCommander*>(CommonSingletonServer::m_iBFCommander)
                    ->sendDefendMessage((*i)->m_sideIndex + 1, (*i)->m_squadIndex + 1);
                break;
            }
            delete *i;
            m_commanderMessages.erase(i++);
        }
        else
            ++i;
    }
}

// Keep the first invalid timestamp; repeated invalid
// reports do not refresh it. A valid report erases the entry, if any. One getId call.
void SquadManager::setPlayerOnValidPosition(IAIPlayer* player, bool valid, double time)
{
    const int id = player->getId();
    std::map<int, double>::iterator i = m_playerValidPositions.find(id);
    if (valid)
    {
        if (i != m_playerValidPositions.end())
            m_playerValidPositions.erase(i);
    }
    else if (i == m_playerValidPositions.end())
        m_playerValidPositions.insert(std::make_pair(id, time));
}

// Copy the borrowed factory
// vector before callbacks. Preserve the repeated AI-player query even when the first one
// returned true. A live, non-downed human with a vehicle next needs IAIObject::getInformation
// (ELF +0xB8 / PE +0xB4); its full interface remains unknown. Stop at that caller boundary.
void SquadManager::updateHumanPlayers(double time)
{
    const std::vector<IAIPlayer*> players =
        static_cast<IAIPlayerFactory*>(CommonSingletonServer::m_iaiPlayerFactory)->getAllPlayers();
    for (std::vector<IAIPlayer*>::const_iterator i = players.begin(); i != players.end(); ++i)
    {
        IAIPlayer* player = *i;
        if (!player)
            continue;
        if (!player->getIsAIPlayer() && player->getIsAlive() && !player->isManDown())
        {
            if (player->getCurrentVehicle())
            {
                player->getSide();
                // [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
                BF2_AI_FRONTIER("dice::hfe::ai::SquadManager::updateHumanPlayers (IAIObject::getInformation)");
            }
        }
        if (!player->getIsAIPlayer())
            setPlayerOnValidPosition(player, true, time);
    }
    for (std::map<int, double>::iterator i = m_playerValidPositions.begin();
         i != m_playerValidPositions.end(); )
    {
        IAIPlayer* player = static_cast<IAIPlayerFactory*>(CommonSingletonServer::m_iaiPlayerFactory)
            ->getPlayerFromId(i->first);
        if (!player || player->getIsAIPlayer() || !player->getIsAlive() || player->isManDown())
            m_playerValidPositions.erase(i++);
        else
            ++i;
    }
}

// Creates a coordinator for each squad on each side, excluding squad zero.
SquadManager::SquadManager()
    : m_coordinators()
    // Default-initialize SideBlock: its three Vec3 values alone are zeroed. Value-initializing
    // the blocks would also zero timestamps/flags retail leaves untouched (full //PE reads).
    , m_commanderMessages()
    , m_playerValidPositions()
{
    const int squads =
        static_cast<IAIGame*>(CommonSingletonServer::m_iaiGame)->getMaxNumberOfSquads();

    if (squads != 1)
    {
        const unsigned int last = static_cast<unsigned int>(squads) - 1;

        for (unsigned int i = 0; i < last; ++i)
        {
            m_coordinators[0].push_back(new SquadCoordinator(0, static_cast<int>(i)));
            m_coordinators[1].push_back(new SquadCoordinator(1, static_cast<int>(i)));
        }
    }

    if (!m_instance)
        m_instance = this;

    m_squadCursor = 0;
    m_float152 = 40.0f;
    m_sideCursor = 1;

    m_sideBlock0.m_b0 = false;
    m_sideBlock1.m_b0 = false;

    m_double156 = 25.0;

    clearCommanderMessages();
    clearPlayerValidPositions();
}

// Deletes each owned commander message before erasing its list node.
// Advances the iterator before erasure.
void SquadManager::clearCommanderMessages()
{
    for (std::list<CommanderMessage*>::iterator i = m_commanderMessages.begin();
         i != m_commanderMessages.end(); )
    {
        delete *i;
        m_commanderMessages.erase(i++);
    }
}

// Erases position records individually, preserving per-node tree rebalancing.
void SquadManager::clearPlayerValidPositions()
{
    for (std::map<int, double>::iterator i = m_playerValidPositions.begin();
         i != m_playerValidPositions.end(); )
    {
        m_playerValidPositions.erase(i++);
    }
}

// complete instructions read on both ports. Prefer a nonempty
// least-populated coordinator, with later entries winning ties. Keep the repeated size reads.
// There is no side-range, empty-roster or player-null guard in either image.
void SquadManager::addSquadMember(int playerId, int side)
{
    --side;
    IAIPlayer* player = static_cast<IAIPlayerFactory*>(CommonSingletonServer::m_iaiPlayerFactory)
        ->getPlayerFromId(playerId);
    SquadCoordinator* selected = 0;
    const int maximum = static_cast<IAIGame*>(CommonSingletonServer::m_iaiGame)
        ->getMaxNumberOfSquadMembers();
    int best = maximum;
    std::vector<SquadCoordinator*>& coordinators = m_coordinators[side];
    for (unsigned int i = 0; i < coordinators.size(); ++i)
    {
        if (coordinators[i]->getNumberOfSquadMembers() <= best &&
            coordinators[i]->getNumberOfSquadMembers() > 0)
        {
            best = coordinators[i]->getNumberOfSquadMembers();
            selected = coordinators[i];
        }
    }
    if (!selected)
        selected = coordinators[0];

    if (static_cast<unsigned int>(selected->getSquadId()) != coordinators.size() - 1 &&
        coordinators[selected->getSquadId() + 1]->getNumberOfSquadMembers() == 0)
    {
        int splitAt = 3;
        const std::string mode = settings->stringGet("GSGameMode", "gpm_cq");
        if (compareIgnoreCase(mode, std::string("gpm_coop")) == 0)
            splitAt = maximum - 1;
        best = selected->getNumberOfSquadMembers();
        if (best >= splitAt)
        {
            // The old member is fetched before removal. Its removal/formation reindexing
            // engine exit/rejoin and subsequent deletion remain unrecovered as a chain.
            selected->getSquadMember(best - 1);
            // [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
            BF2_AI_FRONTIER("dice::hfe::ai::SquadManager::addSquadMember (SquadCoordinator::removeSquadMember)");
        }
    }

    selected->addSquadMember(player);
    Bot* bot = IBotManager::instance->getBotFromPlayerId(playerId);
    if (bot)
    {
        const int squad = selected->getSquadId() + 1;
        bot->getBotSquad()->joinSquad(squad);
    }
    else
    {
        static_cast<IBFSquad*>(CommonSingletonServer::m_iBFSquad)
            ->joinSquad(selected->getSquadId() + 1, player);
    }
    // Both shipped callers discard the bridge's Boolean; a refusal still reaches this update.
    selected->updateSquadLeader();
}

// Both identifiers are one-based. Retail has no range
// singleton or coordinator-null check here; the courier owns only the singleton guard.
void SquadManager::newSquadLeader(int side, int squadId)
{
    m_coordinators[side - 1][squadId - 1]->updateSquadLeader();
}

// Search this side in coordinator order and return the first
// matching member. The coordinator's output index is intentionally not used by this caller.
SquadMember* SquadManager::getSquadMember(int playerId, int side)
{
    std::vector<SquadCoordinator*>& coordinators = m_coordinators[side - 1];
    for (unsigned int i = 0; i < coordinators.size(); ++i)
    {
        int memberIndex;
        SquadMember* member = coordinators[i]->getSquadMember(playerId, memberIndex);
        if (member)
            return member;
    }
    return 0;
}

// The side is one-based and unchecked. Keep the live
// unsigned-index scan and reload the coordinator entry after a successful member query.
// The member index output is not consumed, and null coordinator entries are not guarded.
SquadCoordinator* SquadManager::getSquadFromPlayer(int playerId, int side)
{
    std::vector<SquadCoordinator*>& coordinators = m_coordinators[side - 1];
    for (unsigned int i = 0; i < coordinators.size(); ++i)
    {
        int memberIndex;
        if (coordinators[i]->getSquadMember(playerId, memberIndex))
            return coordinators[i];
    }
    return 0;
}

// Squad zero is ignored. An existing member anywhere on the
// side also returns immediately: a bot's engine join notification must not insert it twice.
// On a miss, retain the unchecked coordinator/player lookups, add, then update the leader.
void SquadManager::playerJoinsSquad(int playerId, int squadId, int side)
{
    if (squadId == 0)
        return;
    if (getSquadMember(playerId, side))
        return;

    SquadCoordinator* coordinator = m_coordinators[side - 1][squadId - 1];
    IAIPlayer* player = static_cast<IAIPlayerFactory*>(CommonSingletonServer::m_iaiPlayerFactory)
        ->getPlayerFromId(playerId);
    coordinator->addSquadMember(player);
    coordinator->updateSquadLeader();
}

} // namespace ai
} // namespace hfe
} // namespace dice
