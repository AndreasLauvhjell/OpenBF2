#include "KickBanEvent.h"

#include "../IGameLogic.h"                       // gameLogic->isServer() - the ctor's loopback gate
#include "../../Core/Debug.h"                    // executeServer's abort
#include "../../Core/EventManager.h"             // eventManager->postEvent
#include "../../IO/BitStream.h"                  // readBits / writeBits
#include "../../Scene/Object/PlayerManager.h"    // getPlayerFromId

namespace dice {
namespace hfe {

namespace {
// .rodata dword GEKickBan (0x8889B84) = 55.
const int GEKickBan = 55;

// The notification executeClient raises: category ECHud (2), event id 0x44 = 68.
const unsigned int NOTIFY_KICK_BAN = 0x44u;
} // namespace

// Stores the event payload, then applies the server's local notification when appropriate.
KickBanEvent::KickBanEvent(int playerId, bool field10, int field14)
    : GameEvent()
    , m_playerId(playerId)                                   // +0x0C
    , m_field10(field10)                                     // +0x10
    , m_field14(field14)                                     // +0x14
{
    // Servers apply the notification immediately, including on dedicated servers.
    if (gameLogic->isServer())
        executeClient();
}

int KickBanEvent::getType() const
{
    return GEKickBan;
}

// Posts the event's own three fields to HUD after checking the player exists.
// The listener must consume this borrowed payload before the event is released.
void KickBanEvent::executeClient()
{
    if (!world::playerManager->getPlayerFromId(m_playerId))              // +0x20
        return;

    eventManager->postEvent(ECHud, NOTIFY_KICK_BAN, &m_playerId, 0.0f);  // +0x14
}

// Rejects client-originated kick/ban events with a fatal diagnostic. The return is unreachable.
bool KickBanEvent::executeServer(int)
{
    Debug(DEBUGTYPE_6, "Game/GameEvents/KickBanEvent.cpp", 97, "")
        << "Server shouldnt get KickBanEvents from outside.";
    return true;
}

// THE bool IS read IN place. `readBits(stream, this + 0x10, 1)` writes the byte at +0x10
// directly - no temporary, no `== 1` comparison, unlike every other bool in this family. Both
// halves do it the same way, so the round trip is consistent; it is reproduced rather than
// normalised because reading one bit into a bool's storage is exactly what the binary does.
bool KickBanEvent::deSerialize(io::BitStream* stream)
{
    int playerId = 0;
    stream->readBits(&playerId, 8);
    m_playerId = playerId;                                   // 8 bits into an int

    stream->readBits(&m_field10, 1);                         // straight into the bool - see above

    int field14 = 0;
    stream->readBits(&field14, 8);
    m_field14 = field14;                                     // 8 bits into an int

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

// serialize - 0x8075850, 195 B. The mirror, bool written straight from its storage.
bool KickBanEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int playerId = m_playerId;
    stream->writeBits(&playerId, 8);

    stream->writeBits(&m_field10, 1);                        // straight from the bool

    int field14 = m_field14;
    stream->writeBits(&field14, 8);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
