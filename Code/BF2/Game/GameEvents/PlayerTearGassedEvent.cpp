#include "PlayerTearGassedEvent.h"

#include "../IGame.h"                            // game->queryInterface(IID_IGameServer)
#include "../GameLogic/Player.h"                 // setBlurryVision - the +0x2B4 slot
#include "../../Core/Ids.h"                      // IID_IGameServer
#include "../../IO/BitStream.h"                  // readBits / writeBits
#include "../../Scene/Object/PlayerManager.h"    // getPlayerFromId

namespace dice {
namespace hfe {

namespace {
// .rodata dword GEPlayerTearGassedEvent (0x8889F48) = 65.
const int GEPlayerTearGassedEvent = 65;
} // namespace


PlayerTearGassedEvent::PlayerTearGassedEvent(int playerId, float gasDamage, float f14, float f18)
    : GameEvent()
    , m_playerId(playerId)                                   // +0x0C
    , m_gasDamage(gasDamage)                                 // +0x10
    , m_field14(f14)                                         // +0x14
    , m_field18(f18)                                         // +0x18
{
}

int PlayerTearGassedEvent::getType() const
{
    return GEPlayerTearGassedEvent;
}

// empty. The client that sends this already applied the effect to itself inside
// Player::setBlurryVision before raising the event; there is nothing for it to do on receipt.
void PlayerTearGassedEvent::executeClient()
{
}

// Applies tear gas to the packet's player ID; connectionId is ignored. This does
// not authenticate that player's ownership. setBlurryVision handles rebroadcast,
// accumulation, and the local player's sound.
bool PlayerTearGassedEvent::executeServer(int /*connectionId*/)
{
    if (!game->queryInterface(IID_IGameServer))
        return false;

    world::IPlayer* player = world::playerManager->getPlayerFromId(m_playerId);      // +0x20
    if (!player)
        return false;

    static_cast<Player*>(player)->setBlurryVision(m_gasDamage, m_field14, m_field18);  // +0x2B4
    return true;
}

// deSerialize - 0x80774B0, 167 B. Eight bits for the id, then three raw 32-bit floats.
// THE floats GO over uncompressed. Every other float-carrying event in this family reaches for
// BitStream's compressed-vector or quaternion paths; this one writes all three at full width
// (`readBits(this + 0x10, 0x20)` and friends). Reproduced.
bool PlayerTearGassedEvent::deSerialize(io::BitStream* stream)
{
    int playerId = 0;
    stream->readBits(&playerId, 8);
    m_playerId = playerId;                                   // 8 bits into an int - see the header

    stream->readBits(&m_gasDamage, 32);
    stream->readBits(&m_field14, 32);
    stream->readBits(&m_field18, 32);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

bool PlayerTearGassedEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int playerId = m_playerId;
    stream->writeBits(&playerId, 8);

    stream->writeBits(&m_gasDamage, 32);
    stream->writeBits(&m_field14, 32);
    stream->writeBits(&m_field18, 32);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
