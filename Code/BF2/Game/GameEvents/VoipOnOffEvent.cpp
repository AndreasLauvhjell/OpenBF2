#include "VoipOnOffEvent.h"

#include "../IGame.h"                            // isPlayingDemo, queryInterface
#include "../IGameServer.h"                      // updatePlayersVoipStatusOnAllClients
#include "../GameLogic/Player.h"                 // setVoipEnabled - the +0x290 slot
#include "../../Core/Ids.h"                      // IID_IGameServer
#include "../../IO/BitStream.h"                  // readBits / writeBits
#include "../../Scene/Object/PlayerManager.h"    // getPlayerFromId / getLocalHumanPlayer

namespace dice {
namespace hfe {

namespace {
// .rodata dword GEVoipOnOff (0x888AE44) = 35.
const int GEVoipOnOff = 35;
} // namespace

// [INFERRED] argument order - no ELF constructor exists.
VoipOnOffEvent::VoipOnOffEvent(int playerId, bool voipEnabled)
    : GameEvent()
    , m_playerId(static_cast<unsigned char>(playerId))
    , m_voipEnabled(voipEnabled)
{
}

int  VoipOnOffEvent::getType() const { return GEVoipOnOff; }
void VoipOnOffEvent::logInfo(bool, int) {}

// Skips demo playback and the local player's own VOIP state. Performs both player
// lookups before null checks and compares IDs, not pointers. The interface declares
// executeClient void, so the original implementation's result is not exposed.
void VoipOnOffEvent::executeClient()
{
    if (game->isPlayingDemo())                                                        // +0xE0
        return;

    world::IPlayer* player = world::playerManager->getPlayerFromId(m_playerId);        // +0x20
    world::IPlayer* local  = world::playerManager->getLocalHumanPlayer();              // +0x30

    if (!player || !local)
        return;

    if (player->getId() != local->getId())                                            // +0x54
        static_cast<Player*>(player)->setVoipEnabled(m_voipEnabled);                  // +0x290
}

// executeServer - 0x8081390, 79 B. A pure relay: hand the pair to the GameServer, which tells every
// connected client. Nothing is written to a Player here.
// THE packet'S player ID IS passed through unchecked - the connection id is never read. A client
// can therefore announce anybody's VOIP status. Reproduced.
bool VoipOnOffEvent::executeServer(int /*connectionId*/)
{
    IGameServer* server = static_cast<IGameServer*>(game->queryInterface(IID_IGameServer));
    if (!server)
        return false;

    server->updatePlayersVoipStatusOnAllClients(m_playerId, m_voipEnabled);           // +0x68
    return true;
}

bool VoipOnOffEvent::deSerialize(io::BitStream* stream)
{
    int playerId = 0;
    stream->readBits(&playerId, 8);
    m_playerId = static_cast<unsigned char>(playerId);

    int voipEnabled = 0;
    stream->readBits(&voipEnabled, 1);
    m_voipEnabled = (voipEnabled == 1);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

bool VoipOnOffEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int playerId = m_playerId;
    stream->writeBits(&playerId, 8);

    int voipEnabled = m_voipEnabled ? 1 : 0;
    stream->writeBits(&voipEnabled, 1);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
