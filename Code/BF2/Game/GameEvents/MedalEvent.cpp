#include "MedalEvent.h"

#include "../IGame.h"                            // game->queryInterface(IID_IGameServer)
#include "../IGameLogic.h"                       // gameLogic->isServer() - the ctor's loopback gate
#include "../../Core/EventManager.h"             // eventManager->postEvent
#include "../../Core/Ids.h"                      // IID_IGameServer
#include "../../IO/BitStream.h"                  // readBits / writeBits
#include "../../Scene/Object/PlayerManager.h"    // getLocalHumanPlayer

namespace dice {
namespace hfe {

namespace {
// .rodata dword GEMedal (0x8889D88) = 41.
const int GEMedal = 41;

// The notification executeClient raises: category ECHud (2), event id 0x27 = 39.
const unsigned int NOTIFY_MEDAL = 0x27u;

// HUD payload order differs from the event: field14, medal, field10.
// [UNRESOLVED] Original payload type name.
struct MedalNotification
{
    int field14;
    int medal;
    int field10;
};
} // namespace

// Stores the second argument in field14 and the third in field10.
MedalEvent::MedalEvent(world::Medal medal, int field14, int field10)
    : GameEvent()
    , m_medal(static_cast<int>(medal))                       // +0x0C
    , m_field10(field10)                                     // +0x10  <- argument 3
    , m_field14(field14)                                     // +0x14  <- argument 2
{
    // A stricter loopback than KickBanEvent'S - see the header. Both halves must hold, so this
    // is inert on a dedicated server.
    if (gameLogic->isServer() && world::playerManager->getLocalHumanPlayer())   // +0x30
        executeClient();
}

int MedalEvent::getType() const
{
    return GEMedal;
}

// executeClient - 0x80769C0, 84 B. Repack the three fields and post them to the HUD. No player
// lookup, no guard of any kind - the notification goes out unconditionally.
void MedalEvent::executeClient()
{
    MedalNotification notification;
    notification.field14 = m_field14;
    notification.medal   = m_medal;
    notification.field10 = m_field10;

    eventManager->postEvent(ECHud, NOTIFY_MEDAL, &notification, 0.0f);   // +0x14
}

// executeServer - 0x8076A20, 41 B. Nothing but the query - see the header block. The `setnz al`
// at 0x8076A3F is the whole conversion; there is no branch and no second call.
bool MedalEvent::executeServer(int /*connectionId*/)
{
    return game->queryInterface(IID_IGameServer) != 0;
}

// deSerialize - 0x8076850, 158 B. Field order 0x0C, 0x14, 0x10 - and widths 32, 8, 4.
bool MedalEvent::deSerialize(io::BitStream* stream)
{
    int medal = 0;
    stream->readBits(&medal, 32);
    m_medal = medal;

    int field14 = 0;
    stream->readBits(&field14, 8);
    m_field14 = field14;

    int field10 = 0;
    stream->readBits(&field10, 4);
    m_field10 = field10;

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

// serialize - 0x80768F0, 201 B. The same order and the same three widths.
bool MedalEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int medal = m_medal;
    stream->writeBits(&medal, 32);

    int field14 = m_field14;
    stream->writeBits(&field14, 8);

    int field10 = m_field10;
    stream->writeBits(&field10, 4);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
