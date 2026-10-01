#include "VerifyPlayerTeamEvent.h"

#include "FixPlayerTeamEvent.h"                  // executeServer's answer
#include "../IGame.h"                            // sendGameEvent
#include "../Objects/IPlayer.h"                  // getId / getTeam
#include "../../IO/BitStream.h"                  // readBits / writeBits
#include "../../Scene/Object/PlayerManager.h"    // getPlayerFromId

namespace dice {
namespace hfe {

namespace {
// .rodata dword GEVerifyPlayerTeam (0x888ADA0) = 68.
const int GEVerifyPlayerTeam = 68;
} // namespace

//  (C2) / 0x8080D00 (C1), 101 B. See the header for the raw store map.
VerifyPlayerTeamEvent::VerifyPlayerTeamEvent(world::IPlayer* localPlayer, world::IPlayer* player)
    : GameEvent()
    , m_playerTeamId(static_cast<unsigned char>(player->getTeam()))            // +0x0C  (+0xE8)
    , m_localPlayerTeamId(static_cast<unsigned char>(localPlayer->getTeam()))  // +0x0D  (+0xE8)
    , m_playerId(static_cast<unsigned char>(player->getId()))                  // +0x0E  (+0x54)
    , m_localPlayerId(static_cast<unsigned char>(localPlayer->getId()))        // +0x0F  (+0x54)
{
}

int  VerifyPlayerTeamEvent::getType() const { return GEVerifyPlayerTeam; }
void VerifyPlayerTeamEvent::logInfo(bool, int) {}

// Performs the two player lookups without changing state. The original Boolean
// validity result is not exposed by this interface's void executeClient signature.
void VerifyPlayerTeamEvent::executeClient()
{
    world::playerManager->getPlayerFromId(m_playerId);        // +0x20
    world::playerManager->getPlayerFromId(m_localPlayerId);   // +0x20
    // nothing is done with either - see above.
}

// Sends a team correction to m_localPlayerId only when the subject's team differs.
// Ignores connectionId; the payload selects the recipient. Missing players return
// false, while an already-correct team returns true without sending.
bool VerifyPlayerTeamEvent::executeServer(int /*connectionId*/)
{
    world::IPlayer* player = world::playerManager->getPlayerFromId(m_playerId);        // +0x20
    world::IPlayer* local  = world::playerManager->getPlayerFromId(m_localPlayerId);   // +0x20

    if (!player || !local)
        return false;

    if (player->getTeam() != m_playerTeamId)                                           // +0xE8
    {
        game->sendGameEvent(m_localPlayerId,
                            new FixPlayerTeamEvent(local, player));                    // +0x90
    }

    return true;
}

// deSerialize - 0x8080D80, 199 B. Four bytes, in field order.
bool VerifyPlayerTeamEvent::deSerialize(io::BitStream* stream)
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

bool VerifyPlayerTeamEvent::serialize(io::BitStream* stream)
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
