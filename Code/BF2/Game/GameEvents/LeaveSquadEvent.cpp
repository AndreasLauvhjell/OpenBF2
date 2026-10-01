#include "LeaveSquadEvent.h"

#include "../IGame.h"                            // game->queryInterface(IID_IGameServer)
#include "../GameLogic/Player.h"                 // getIsSquadLeader - the permission arm
#include "../GameLogic/SquadManager/ISquadManager.h" // typed SquadManager +0x34 dispatch
#include "../../Core/Ids.h"                      // IID_IGameServer
#include "../../IO/BitStream.h"                  // readBits / writeBits
#include "../../Scene/Object/PlayerManager.h"    // getPlayerFromId

namespace dice {
namespace hfe {

extern IBase* squadManager;                      // ds:0x8ACBFA4 - Core/Globals.cpp (IBase*, no layout)

namespace {
// .rodata dword GELeaveSquad (0x8889D04) = 17.
const int GELeaveSquad = 17;

} // namespace

// [INFERRED] argument order - no ELF constructor exists.
LeaveSquadEvent::LeaveSquadEvent(int team, int playerId, int squadId, bool clearInvite)
    : JoinSquadEvent(team, playerId, squadId)
    , m_clearInvite(clearInvite)
{
}

int LeaveSquadEvent::getType() const
{
    return GELeaveSquad;
}

// executeClient - 0x80760C0, 10 B. An explicit empty override, exactly as in the base - the client
// learns about the removal from the SquadManager's own broadcast, not from this event.
void LeaveSquadEvent::executeClient()
{
}

// Removes the payload-named player when the sender is that player, a squad leader,
// or unexpectedly absent from the player registry. Returns false outside a server
// or when authorization fails; the unknown-sender allowance is original behavior.
bool LeaveSquadEvent::executeServer(int connectionId)
{
    if (!game->queryInterface(IID_IGameServer))
        return false;

    world::IPlayer* sender = world::playerManager->getPlayerFromId(connectionId);   // +0x20

    const bool allowed = (m_playerId == connectionId)                               // (a)
                      || (sender == 0)                                              // (b) fail-open
                      || static_cast<Player*>(sender)->getIsSquadLeader();          // (c) +0x13C

    if (!allowed)
        return false;

    if (squadManager)
        static_cast<ISquadManager*>(squadManager)->removeFromSquad(
            m_team, m_squadId, m_playerId, m_clearInvite);                         // +0x34

    return true;
}

// deSerialize - 0x8076180, 199 B. The base's three bytes, then one bit.
// THE bool IS decoded AS `== 1`, NOT AS `!= 0`. It is a one-bit field so the two agree, but the
// ELF really does compare against the literal 1 and that is what is written here.
bool LeaveSquadEvent::deSerialize(io::BitStream* stream)
{
    int team = 0;
    stream->readBits(&team, 8);
    m_team = static_cast<unsigned char>(team);

    int playerId = 0;
    stream->readBits(&playerId, 8);
    m_playerId = static_cast<unsigned char>(playerId);

    int squadId = 0;
    stream->readBits(&squadId, 8);
    m_squadId = static_cast<unsigned char>(squadId);

    int clearInvite = 0;
    stream->readBits(&clearInvite, 1);
    m_clearInvite = (clearInvite == 1);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

bool LeaveSquadEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int team = m_team;
    stream->writeBits(&team, 8);

    int playerId = m_playerId;
    stream->writeBits(&playerId, 8);

    int squadId = m_squadId;
    stream->writeBits(&squadId, 8);

    int clearInvite = m_clearInvite ? 1 : 0;
    stream->writeBits(&clearInvite, 1);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
