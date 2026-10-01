// Client squad requests either forward locally or send the matching game event
// with player ID -1. inviteToSquad uses the connected-send helper. Source path is attested.

#include "ClientSquadManager.h"

#include "../../IGame.h"                              // game->sendGameEvent / sendGameEventIfConnected
#include "../../IGameLogic.h"                         // gameLogic - tested by the commander-camera modes
#include "../../Objects/IPlayer.h"                    // getId / getTeam / setIsCommander / getIsCommander /
                                                      //   setOverheadCameraEnabled
#include "../../../Scene/Object/PlayerManager.h"      // world::playerManager - getLocalHumanPlayer / getPlayerFromId
#include "../../../Core/Debug.h"                      // Debug(DEBUGTYPE_6, ...) - the two "should not be called" sites
#include "../../../Core/Ids.h"                        // IID_IBase, IID_ISquadManager
#include "../../GameEvents/SetSquadLeaderEvent.h"
#include "../../GameEvents/ChangeSquadNameEvent.h"
#include "../../GameEvents/SetPrivateSquadEvent.h"
#include "../../GameEvents/SetAcceptOrderEvent.h"
#include "../../GameEvents/IssueSquadOrderEvent.h"
#include "../../GameEvents/JoinSquadEvent.h"
#include "../../GameEvents/LeaveSquadEvent.h"
#include "../../GameEvents/InviteEvent.h"
#include "../../GameEvents/CommanderEvent.h"

namespace dice {
namespace hfe {

namespace {
// The binary sends every request with player id -1 through IGame's `sendGameEvent(unsigned int,
// GameEvent*)`. Written once.
const unsigned int kBroadcastPlayerId = static_cast<unsigned int>(-1);
}

// construction - 0x8117C30 (C2), 54 B: SquadManager(), then the refcount and the two commander slots.
ClientSquadManager::ClientSquadManager()
    : m_refCount(1)
{
    m_commanderPlayerId[0] = -1;                                  // +0x2C
    m_commanderPlayerId[1] = -1;                                  // +0x30
}

//  (D2), 41 B: drop the squads, then ~SquadManager.
ClientSquadManager::~ClientSquadManager()
{
    removeAllSquads();
}

// the IBase quartet - 0x8118EA0 / 0x8118EB0 / 0x8118EC0 / 0x8118F00. release keeps the shipped
// `r == 0 && this` test; queryInterface answers IID_IBase and IID_ISquadManager with `this`.
void ClientSquadManager::addRef()  { ++m_refCount; }
int  ClientSquadManager::getRef()  { return m_refCount; }
int  ClientSquadManager::release()
{
    const int r = --m_refCount;
    if (r == 0 && this)
        delete this;                                              // vt +0x9C = D0, the deleting destructor
    return r;
}
void* ClientSquadManager::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase)
        return const_cast<ClientSquadManager*>(this);
    if (iid == IID_ISquadManager)
        return const_cast<ClientSquadManager*>(this);
    return 0;
}

// create - 0x8531310, 89 B. The aggregation-aware factory shape shared with SquadManager::create and
// ServerGameLogic::create: refuse an outer, new + construct (0x34), answer queryInterface(iid),
// release the half-built object when the interface is not supported.
IBase* ClientSquadManager::create(unsigned int iid, IBase* outer)
{
    if (outer)
        return 0;
    ClientSquadManager* manager = new ClientSquadManager;         // operator new(0x34)
    IBase* result = static_cast<IBase*>(manager->queryInterface(iid));   // vt +0x0C
    if (!result)
        manager->release();                                       // vt +0x08
    return result;
}

// the squad requests

// . `local` -> SquadManager::setSquadLeader; otherwise a SetSquadLeaderEvent
// (team, playerId, squadId - the JoinSquadEvent layout) to the server.
void ClientSquadManager::setSquadLeader(int team, int squadId, int playerId, bool local)
{
    if (local)
    {
        SquadManager::setSquadLeader(team, squadId, playerId, local);
        return;
    }
    if (game)
    {
        SetSquadLeaderEvent* event = new SetSquadLeaderEvent(team, playerId, squadId);   // operator new(0x10)
        game->sendGameEvent(kBroadcastPlayerId, event);           // IGame +0x90
    }
}

// Requires a local human. Local requests return SquadManager's result; remote
// requests must name that player and return true once accepted even if game is absent.
bool ClientSquadManager::setSquadName(int team, int squadId, int playerId, const std::string& name, bool local)
{
    if (!world::playerManager->getLocalHumanPlayer())             // vt +0x30
        return false;
    if (local)
        return SquadManager::setSquadName(team, squadId, playerId, name, local);
    if (playerId == world::playerManager->getLocalHumanPlayer()->getId())   // IPlayer +0x54
    {
        if (game)
        {
            ChangeSquadNameEvent* event = new ChangeSquadNameEvent(team, squadId, playerId, name);   // operator new(0x30)
            game->sendGameEvent(kBroadcastPlayerId, event);
        }
        return true;
    }
    return false;
}

void ClientSquadManager::setIsPrivate(int team, int squadId, bool isPrivate, bool local)
{
    if (local)
    {
        SquadManager::setIsPrivate(team, squadId, isPrivate, local);
        return;
    }
    if (game)
    {
        SetPrivateSquadEvent* event = new SetPrivateSquadEvent(team, squadId, isPrivate);   // operator new(0x10)
        game->sendGameEvent(kBroadcastPlayerId, event);
    }
}

// . The forwarded form returns SquadManager's answer; the sent form returns true.
bool ClientSquadManager::setIsAccepted(int team, int squadId, bool accepted, OrderType orderType,
                                       SquadOrderType squadOrderType, bool local)
{
    if (local)
        return SquadManager::setIsAccepted(team, squadId, accepted, orderType, squadOrderType, local);
    if (game)
    {
        GameEvent* event = new SetAcceptOrderEvent(team, squadId, accepted, orderType, squadOrderType);   // operator new(0x14)
        game->sendGameEvent(kBroadcastPlayerId, event);
    }
    return true;
}

//  / 0x8118110 / 0x81181B0, 153-156 B each: the same body with the IssueSquadOrderEvent's
// kind at 0 (squad order), 1 (commander order), 2 (request). The OrderType enumerators are not
// named in this tree ([UNRESOLVED]); the literals are the binary's.
void ClientSquadManager::issueSquadOrder(int team, int squadId, SquadOrder& order, bool local)
{
    if (local)
    {
        SquadManager::issueSquadOrder(team, squadId, order, local);
        return;
    }
    if (game)
    {
        GameEvent* event = new IssueSquadOrderEvent(team, squadId, &order, static_cast<OrderType>(0));   // operator new(0x38)
        game->sendGameEvent(kBroadcastPlayerId, event);
    }
}

void ClientSquadManager::issueCommanderOrder(int team, int squadId, SquadOrder& order, bool local)
{
    if (local)
    {
        SquadManager::issueCommanderOrder(team, squadId, order, local);
        return;
    }
    if (game)
    {
        GameEvent* event = new IssueSquadOrderEvent(team, squadId, &order, static_cast<OrderType>(1));
        game->sendGameEvent(kBroadcastPlayerId, event);
    }
}

void ClientSquadManager::issueRequest(int team, int squadId, SquadOrder& order, bool local)
{
    if (local)
    {
        SquadManager::issueRequest(team, squadId, order, local);
        return;
    }
    if (game)
    {
        GameEvent* event = new IssueSquadOrderEvent(team, squadId, &order, static_cast<OrderType>(2));
        game->sendGameEvent(kBroadcastPlayerId, event);
    }
}

//  / PE 0x462FA0. Sends JoinSquadEvent when game exists, then returns true on both arms.
bool ClientSquadManager::addToSquad(int team, int squadId, int playerId)
{
    if (game)
    {
        JoinSquadEvent* event = new JoinSquadEvent(team, playerId, squadId);   // operator new(0x10)
        game->sendGameEvent(kBroadcastPlayerId, event);
    }
    return true;
}

// . No local arm: a LeaveSquadEvent (team, playerId, squadId, clearInvite).
void ClientSquadManager::removeFromSquad(int team, int squadId, int playerId, bool clearInvite)
{
    if (game)
    {
        LeaveSquadEvent* event = new LeaveSquadEvent(team, playerId, squadId, clearInvite);   // operator new(0x10)
        game->sendGameEvent(kBroadcastPlayerId, event);
    }
}

// . No `game` test and the "if connected" send (IGame +0x98); the fifth
// argument (suppress) is not read.
void ClientSquadManager::inviteToSquad(int team, int squadId, int playerId, bool invited, bool /*suppress*/)
{
    InviteEvent* event = new InviteEvent(team, playerId, squadId, invited);   // operator new(0x10)
    game->sendGameEventIfConnected(kBroadcastPlayerId, event);
}

// . A server-only query: logged, and -1.
int ClientSquadManager::getFirstUnusedSquad(int /*team*/) const
{
    Debug(DEBUGTYPE_6, "Game/GameLogic/SquadManager/ClientSquadManager.cpp", 195)
        << "ClientSquadManager::getFirstUnusedSquad should not be called on client.";
    return -1;
}

// the commander requests - CommanderEvents. Only two of the seven type ids the client sends are
// named in Game/GameEvents/CommanderEvent.h (6 soldier view, 7 overhead view); the others are
// transcribed as the binary's literals: 4 apply, 0 accept, 1 decline, 2 resign, 8 zoom mode.

void ClientSquadManager::applyForCommander(int team, int playerId)
{
    if (game)
    {
        CommanderEvent* event = new CommanderEvent(static_cast<CommanderEvent::CommanderEventTypes>(4), team, playerId);   // operator new(0x14)
        game->sendGameEvent(kBroadcastPlayerId, event);
    }
}

// . Server-only: logged, and -1.
int ClientSquadManager::popNextCommanderApplicant(int /*team*/)
{
    Debug(DEBUGTYPE_6, "Game/GameLogic/SquadManager/ClientSquadManager.cpp", 212)
        << "ClientSquadManager::popNextCommanderApplicant should not be called on client.";
    return -1;
}

void ClientSquadManager::acceptCommander(int team, int playerId)
{
    if (game)
    {
        CommanderEvent* event = new CommanderEvent(static_cast<CommanderEvent::CommanderEventTypes>(0), team, playerId);
        game->sendGameEvent(kBroadcastPlayerId, event);
    }
}

void ClientSquadManager::declineCommander(int team, int playerId)
{
    if (game)
    {
        CommanderEvent* event = new CommanderEvent(static_cast<CommanderEvent::CommanderEventTypes>(1), team, playerId);
        game->sendGameEvent(kBroadcastPlayerId, event);
    }
}

// . The player id is the local player's (asked after the allocation, unguarded).
void ClientSquadManager::resignCommander(int team)
{
    if (game)
    {
        const int playerId = world::playerManager->getLocalHumanPlayer()->getId();   // unguarded, as shipped
        CommanderEvent* event = new CommanderEvent(static_cast<CommanderEvent::CommanderEventTypes>(2), team, playerId);   // operator new(0x14)
        game->sendGameEvent(kBroadcastPlayerId, event);
    }
}

// . Purely local: flip the old and new commanders' IPlayer flags and record the id.
// No event, no `game` test.
void ClientSquadManager::setCommander(int team, int playerId)
{
    world::IPlayer* previous = world::playerManager->getPlayerFromId(m_commanderPlayerId[team - 1]);   // vt +0x20
    world::IPlayer* next     = world::playerManager->getPlayerFromId(playerId);
    if (previous)
        previous->setIsCommander(false);                          // IPlayer +0x110
    if (next)
        next->setIsCommander(true);
    m_commanderPlayerId[team - 1] = playerId;
}

int ClientSquadManager::getCommander(int team)
{
    return m_commanderPlayerId[team - 1];
}

// . Needs `game` and `gameLogic`; the event carries the local player's team and id
// (two more getLocalHumanPlayer calls, unguarded), then the addressed player leaves the overhead
// camera. The playerId argument selects only that last step.
void ClientSquadManager::setCommanderToSoldierMode(int playerId)
{
    if (!game || !gameLogic)
        return;
    const int localId   = world::playerManager->getLocalHumanPlayer()->getId();
    const int localTeam = world::playerManager->getLocalHumanPlayer()->getTeam();     // IPlayer +0xE8
    CommanderEvent* event = new CommanderEvent(CommanderEvent::CESoldierView, localTeam, localId);   // 6, operator new(0x14)
    game->sendGameEvent(kBroadcastPlayerId, event);
    if (world::IPlayer* player = world::playerManager->getPlayerFromId(playerId))
        player->setOverheadCameraEnabled(false);                  // IPlayer +0xB8
}

// . As above with the overhead-view id (7), gated on the local player being the
// commander (IPlayer +0x114); the addressed player enters the overhead camera.
void ClientSquadManager::setCommanderToOverheadCamMode(int playerId)
{
    if (!game || !gameLogic || !world::playerManager)
        return;
    if (!world::playerManager->getLocalHumanPlayer()->getIsCommander())
        return;
    const int localId   = world::playerManager->getLocalHumanPlayer()->getId();
    const int localTeam = world::playerManager->getLocalHumanPlayer()->getTeam();
    CommanderEvent* event = new CommanderEvent(CommanderEvent::CEOverheadView, localTeam, localId);  // 7
    game->sendGameEvent(kBroadcastPlayerId, event);
    if (world::IPlayer* player = world::playerManager->getPlayerFromId(playerId))
        player->setOverheadCameraEnabled(true);
}

// . The same gate, id 8, then SquadManager's own zoom-mode body runs locally too.
void ClientSquadManager::setCommanderOverheadCamZoomMode(int playerId, bool zoom)
{
    if (!game || !gameLogic || !world::playerManager)
        return;
    if (!world::playerManager->getLocalHumanPlayer()->getIsCommander())
        return;
    const int localId   = world::playerManager->getLocalHumanPlayer()->getId();
    const int localTeam = world::playerManager->getLocalHumanPlayer()->getTeam();
    CommanderEvent* event = new CommanderEvent(static_cast<CommanderEvent::CommanderEventTypes>(8), localTeam, localId);
    game->sendGameEvent(kBroadcastPlayerId, event);
    SquadManager::setCommanderOverheadCamZoomMode(playerId, zoom);
}

} // namespace hfe
} // namespace dice
