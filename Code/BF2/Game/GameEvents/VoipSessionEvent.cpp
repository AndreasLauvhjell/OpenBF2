#include "VoipSessionEvent.h"

#include "../IGame.h"                            // isPlayingDemo
#include "../GameLogic/Player.h"                 // setVoipSessionId - the +0x274 slot
#include "../../IO/BitStream.h"                  // readBits / writeBits
#include "../../Scene/Object/PlayerManager.h"    // getPlayerFromId / getLocalHumanPlayer

namespace dice {
namespace hfe {

namespace {
// Wire event ID 54; registration order does not define the protocol ID.
const int GEVoipSession = 54;
} // namespace

// NO ELF address - the constructor was inlined into GameServer::createPlayer at 0x80B0CD8 and
// left no out-of-line symbol. These are the two stores that site makes, in its order; the word
// store `mov [ebx+0Ch], si` is why the parameter is an int and the member is not.
VoipSessionEvent::VoipSessionEvent(int sessionId)
    : GameEvent()
    , m_sessionId(static_cast<unsigned short>(sessionId))
{
}

int VoipSessionEvent::getType() const
{
    return GEVoipSession;
}

// Sets the local player's VOIP session when both manager and player exist.
// Skips demo playback before lookup to avoid adopting recorded session IDs.
void VoipSessionEvent::executeClient()
{
    if (game->isPlayingDemo())                                              // +0xE0
        return;

    if (!world::playerManager)
        return;

    world::IPlayer* local = world::playerManager->getLocalHumanPlayer();    // +0x30
    if (!local)
        return;

    static_cast<Player*>(local)->setVoipSessionId(m_sessionId);             // +0x274
}

// executeServer - 0x8081A90, 72 B. The mirror: record the id against the sending connection.
// NO IID_IGameServer gate and no demo gate - this arm is 72 bytes and goes straight to the
// player manager. It also trusts the connection for identity (getPlayerFromId(connectionId)) and
// ignores nothing, because the event carries no player field to ignore.
bool VoipSessionEvent::executeServer(int connectionId)
{
    if (!world::playerManager)
        return false;

    world::IPlayer* player = world::playerManager->getPlayerFromId(connectionId);   // +0x20
    if (!player)
        return false;

    static_cast<Player*>(player)->setVoipSessionId(m_sessionId);                    // +0x274
    return true;
}

// deSerialize - 0x8081940, 87 B. One 16-bit field.
bool VoipSessionEvent::deSerialize(io::BitStream* stream)
{
    int sessionId = 0;
    stream->readBits(&sessionId, 16);
    m_sessionId = static_cast<unsigned short>(sessionId);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

bool VoipSessionEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int sessionId = m_sessionId;
    stream->writeBits(&sessionId, 16);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
