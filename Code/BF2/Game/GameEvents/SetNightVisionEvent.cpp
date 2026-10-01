#include "SetNightVisionEvent.h"

#include "../IGame.h"                            // game->queryInterface(IID_IGameServer)
#include "../GameLogic/Player.h"                 // setNightVision - the +0x2DC slot
#include "../../Core/Ids.h"                      // IID_IGameServer
#include "../../IO/BitStream.h"                  // readBits / writeBits
#include "../../Scene/Object/PlayerManager.h"    // getPlayerFromId

namespace dice {
namespace hfe {

namespace {
// .rodata dword GESetNightVisionEvent (0x888A544) = 66.
const int GESetNightVisionEvent = 66;
} // namespace


SetNightVisionEvent::SetNightVisionEvent(int playerId, bool nightVision)
    : GameEvent()
    , m_playerId(playerId)                                   // +0x0C, a dword store
    , m_nightVision(nightVision)                             // +0x10
{
}

int SetNightVisionEvent::getType() const
{
    return GESetNightVisionEvent;
}

// Intentionally empty client override.
void SetNightVisionEvent::executeClient()
{
}

// Rejects a packet naming anyone other than the transport sender, then looks up
// that sender and applies night vision. The second setter argument remains false.
bool SetNightVisionEvent::executeServer(int connectionId)
{
    if (!game->queryInterface(IID_IGameServer))
        return false;

    if (connectionId != m_playerId)
        return false;

    world::IPlayer* player = world::playerManager->getPlayerFromId(connectionId);   // +0x20
    if (!player)
        return false;

    static_cast<Player*>(player)->setNightVision(m_nightVision, false);             // +0x2DC
    return true;
}

// deSerialize - 0x807AC10, 135 B. Eight bits into the int field, then one bit.
bool SetNightVisionEvent::deSerialize(io::BitStream* stream)
{
    int playerId = 0;
    stream->readBits(&playerId, 8);
    m_playerId = playerId;                                   // 8 bits into an int - see the header

    int nightVision = 0;
    stream->readBits(&nightVision, 1);
    m_nightVision = (nightVision == 1);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

bool SetNightVisionEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int playerId = m_playerId;
    stream->writeBits(&playerId, 8);

    int nightVision = m_nightVision ? 1 : 0;
    stream->writeBits(&nightVision, 1);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
