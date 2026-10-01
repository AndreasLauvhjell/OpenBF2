#include "FixPlayerTeamEvent.h"

#include "../Objects/IPlayer.h"                  // getId / getTeam / setTeam
#include "../../IO/BitStream.h"                  // readBits / writeBits
#include "../../Scene/Object/PlayerManager.h"    // getPlayerFromId / getLocalHumanPlayer

namespace dice {
namespace hfe {

namespace {
// .rodata dword GEFixPlayerTeam (0x8889508) = 69.
const int GEFixPlayerTeam = 69;
} // namespace

//  (C2) / 0x80713D0 (C1), 101 B. Four virtual calls, four byte stores. The store order in
// the ELF is +0x0D, +0x0C, +0x0F, +0x0E - compiler scheduling, not meaning; VerifyPlayerTeamEvent
// does the same four in the order +0x0C, +0x0D, +0x0F, +0x0E. Written in field order here.
FixPlayerTeamEvent::FixPlayerTeamEvent(world::IPlayer* localPlayer, world::IPlayer* player)
    : GameEvent()
    , m_playerTeamId(static_cast<unsigned char>(player->getTeam()))            // +0x0C  (+0xE8)
    , m_localPlayerTeamId(static_cast<unsigned char>(localPlayer->getTeam()))  // +0x0D  (+0xE8)
    , m_playerId(static_cast<unsigned char>(player->getId()))                  // +0x0E  (+0x54)
    , m_localPlayerId(static_cast<unsigned char>(localPlayer->getId()))        // +0x0F  (+0x54)
{
}

int  FixPlayerTeamEvent::getType() const { return GEFixPlayerTeam; }
void FixPlayerTeamEvent::logInfo(bool, int) {}

// Applies a team correction only on the addressed local player's machine. Resolves
// both players before testing them; a changed team uses keepVerifyPending == true.
void FixPlayerTeamEvent::executeClient()
{
    world::IPlayer* player = world::playerManager->getPlayerFromId(m_playerId);        // +0x20
    world::IPlayer* local  = world::playerManager->getPlayerFromId(m_localPlayerId);   // +0x20

    if (!player || !local)
        return;

    if (world::playerManager->getLocalHumanPlayer() != local)                          // +0x30
        return;

    if (m_localPlayerTeamId == m_playerTeamId)
        return;                                                                        // already agreed

    player->setTeam(m_playerTeamId, true);                                             // +0xE4
}

// Overwrites the event's team with server state for the caller to send back.
// Both player lookups must succeed, though only the first player's team is used.
// Does not check a server interface or use connectionId.
bool FixPlayerTeamEvent::executeServer(int /*connectionId*/)
{
    world::IPlayer* player = world::playerManager->getPlayerFromId(m_playerId);        // +0x20
    world::IPlayer* local  = world::playerManager->getPlayerFromId(m_localPlayerId);   // +0x20

    if (!player || !local)
        return false;

    m_playerTeamId = static_cast<unsigned char>(player->getTeam());                    // +0xE8
    return true;
}

// deSerialize - 0x8071450, 231 B. Four bytes, in field order.
bool FixPlayerTeamEvent::deSerialize(io::BitStream* stream)
{
    int playerTeamId = 0;
    stream->readBits(&playerTeamId, 8);
    m_playerTeamId = static_cast<unsigned char>(playerTeamId);

    int localPlayerTeamId = 0;
    stream->readBits(&localPlayerTeamId, 8);
    m_localPlayerTeamId = static_cast<unsigned char>(localPlayerTeamId);

    int playerId = 0;
    stream->readBits(&playerId, 8);
    m_playerId = static_cast<unsigned char>(playerId);

    int localPlayerId = 0;
    stream->readBits(&localPlayerId, 8);
    m_localPlayerId = static_cast<unsigned char>(localPlayerId);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

bool FixPlayerTeamEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int playerTeamId = m_playerTeamId;
    stream->writeBits(&playerTeamId, 8);

    int localPlayerTeamId = m_localPlayerTeamId;
    stream->writeBits(&localPlayerTeamId, 8);

    int playerId = m_playerId;
    stream->writeBits(&playerId, 8);

    int localPlayerId = m_localPlayerId;
    stream->writeBits(&localPlayerId, 8);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
