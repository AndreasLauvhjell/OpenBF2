// [INFERRED] Source placement.
// [FRONTIER] Remaining methods stop by name until implemented.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining BotSquad operations.

#include "BotSquad.h"
#include "BotMain.h"

#include "../Main/AIDllFrontier.h"
#include "../Main/CommonSingletonServer.h"
#include "../../../AIInterface/BFKnowledge/BFSquad.h"

namespace dice {
namespace hfe {
namespace ai {

// Three stores, and the interesting one is the third: a fresh squad handle starts at `OrderNone`
// not at zero. Zero is `OrderAttack`, so a memset-initialised BotSquad would believe it had been
// told to attack. The constructor is what keeps that from being true.
BotSquad::BotSquad(BotMain* owner)
    : m_bot(owner)
    , m_squadId(0)
    , m_order(OrderNone)
{
}

// five bytes each - bare `ret`s, and `reset` doing nothing is a finding rather than a gap.
// A recycled bot keeps its squad id and its last order. See BotSquad.h.
BotSquad::~BotSquad()
{
}

void BotSquad::reset()
{
}

// The four order predicates. One `cmp` each, and between them they are how the enum was read.
bool BotSquad::hasAttackOrder()    { return m_order == OrderAttack; }
bool BotSquad::hasDefendOrder()    { return m_order == OrderDefend; }
bool BotSquad::hasMoveOrderOrder() { return m_order == OrderMove;   }   // dice's name
bool BotSquad::hasNoOrder()        { return m_order == OrderNone;   }

// Forwards squad operations to the engine squad adapter using the bot's player.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.

// Both test and return the adapter's AL.
bool BotSquad::joinSquad(int squad)
{
#if defined(_MSC_VER)
    // [PLATFORM] PE captures the adapter before getPlayer; ELF loads it afterward.
    IBFSquad* const adapter = static_cast<IBFSquad*>(CommonSingletonServer::m_iBFSquad);
    IAIPlayer* const player = m_bot->getPlayer();
#else
    IAIPlayer* const player = m_bot->getPlayer();
    IBFSquad* const adapter = static_cast<IBFSquad*>(CommonSingletonServer::m_iBFSquad);
#endif
    const bool joined = adapter->joinSquad(squad, player);
    if (joined)
        m_squadId = squad;
    return joined;
}

bool BotSquad::createSquad()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotSquad::createSquad");
}

void BotSquad::exitSquad()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotSquad::exitSquad");
}

bool BotSquad::isSquadLeader() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotSquad::isSquadLeader");
}

void BotSquad::applyAsSquadLeader(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotSquad::applyAsSquadLeader");
}

void BotSquad::retireAsSquadLeader(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotSquad::retireAsSquadLeader");
}

void BotSquad::issueSquadAttackOrder(const Vec3&, Information*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotSquad::issueSquadAttackOrder");
}

void BotSquad::issueSquadMoveOrder(const Vec3&, Information*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotSquad::issueSquadMoveOrder");
}

void BotSquad::issueSquadDefendOrder(const Vec3&, Information*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotSquad::issueSquadDefendOrder");
}

void BotSquad::recieveAttackSquadOrder()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotSquad::recieveAttackSquadOrder");
}

void BotSquad::recieveDefendSquadOrder()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotSquad::recieveDefendSquadOrder");
}

void BotSquad::recieveMoveSquadOrder()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotSquad::recieveMoveSquadOrder");
}

void BotSquad::recieveAttackCommanderOrder()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotSquad::recieveAttackCommanderOrder");
}

void BotSquad::recieveDefendCommanderOrder()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotSquad::recieveDefendCommanderOrder");
}

void BotSquad::recieveMoveCommanderOrder()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotSquad::recieveMoveCommanderOrder");
}

void BotSquad::acceptSquadOrder()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotSquad::acceptSquadOrder");
}

void BotSquad::declineSquadOrder()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotSquad::declineSquadOrder");
}

} // namespace ai
} // namespace hfe
} // namespace dice
