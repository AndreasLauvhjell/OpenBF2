#include "InviteEvent.h"

#include "../IGame.h"                            // game->queryInterface(IID_IGameServer)
#include "../GameLogic/SquadManager/ISquadManager.h" // typed SquadManager dispatch
#include "../../Core/Ids.h"                      // IID_IGameServer
#include "../../IO/BitStream.h"                  // readBits / writeBits

namespace dice {
namespace hfe {

extern IBase* squadManager;                      // ds:0x8ACBFA4 - Core/Globals.cpp (IBase*, no layout)

namespace {
// .rodata dword GEInvite (0x88899E0) = 25.
const int GEInvite = 25;

} // namespace

// [INFERRED] argument order - no ELF constructor exists.
InviteEvent::InviteEvent(int team, int playerId, int squadId, bool invited)
    : GameEvent()
    , m_team(static_cast<unsigned char>(team))
    , m_playerId(static_cast<unsigned char>(playerId))
    , m_squadId(static_cast<unsigned char>(squadId))
    , m_invited(invited)
{
}

int InviteEvent::getType() const
{
    return GEInvite;
}

// an explicit empty override. The invitee learns of the invite from the
// SquadManager's own notify path (a PostRemoteEvent with event id 16), not from this class.
void InviteEvent::executeClient()
{
}

// Only the squad leader may issue invitations; non-leaders may withdraw them.
// Authorization compares the full connection id. Accepted changes always notify clients.
bool InviteEvent::executeServer(int connectionId)
{
    if (!game->queryInterface(IID_IGameServer))
        return false;

    if (!squadManager)
        return true;

    const int leader = static_cast<ISquadManager*>(squadManager)->getSquadLeader(
        m_team, m_squadId);                                                           // +0x3C

    bool invited = m_invited;
    if (leader != connectionId)
    {
        // Not the leader: only a withdrawal is allowed through.
        if (invited)
            return false;
        invited = false;
    }

    static_cast<ISquadManager*>(squadManager)->inviteToSquad(
        m_team, m_squadId, m_playerId, invited, false);                               // +0x2C
    return true;
}

bool InviteEvent::deSerialize(io::BitStream* stream)
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

    int invited = 0;
    stream->readBits(&invited, 1);
    m_invited = (invited == 1);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

bool InviteEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int team = m_team;
    stream->writeBits(&team, 8);

    int playerId = m_playerId;
    stream->writeBits(&playerId, 8);

    int squadId = m_squadId;
    stream->writeBits(&squadId, 8);

    int invited = m_invited ? 1 : 0;
    stream->writeBits(&invited, 1);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
