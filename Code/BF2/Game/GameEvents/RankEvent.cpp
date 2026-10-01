#include "RankEvent.h"

#include "../IGameLogic.h"                       // gameLogic->isServer() - the ctor's loopback gate
#include "../Objects/IPlayer.h"                  // getScoreData
#include "../../Core/Debug.h"                    // both abort sites, and the inlined range assert
#include "../../Core/EventManager.h"             // eventManager->postEvent
#include "../../IO/BitStream.h"                  // readBits / writeBits
#include "../../Scene/Object/PlayerManager.h"    // getPlayerFromId / getLocalHumanPlayer
#include "../../Scene/Object/PlayerScoreData.h"  // setRank

namespace dice {
namespace hfe {

namespace {
// .rodata dword GERank (0x888A2A0) = 26.
const int GERank = 26;

// The notification executeClient raises: category ECHud (2), event id 0x26 = 38. One lower than
// MedalEvent's 0x27 - the two sit next to each other in whatever the client's Hud event list is.
const unsigned int NOTIFY_RANK = 0x26u;

// The one value of RankEventTypes this binary can produce - hardcoded by sgl_sendRankEvent, and
// demanded by executeClient. See the header.
const int RANK_TYPE_PROMOTION = 1;

// Local ranged-bit helpers shared in behavior with ConnectionTypeEvent.
// Move both copies together if the common BitStream helper is introduced.
unsigned int bitsToHold(unsigned int highest)
{
    unsigned int bits = 0;
    do
        ++bits;
    while (((1u << bits) - 1u) < highest);
    return bits;
}

void writeRangedBits(io::BitStream* stream, unsigned int value,
                     unsigned int lower, unsigned int upper)
{
    if (value > upper)
    {
        stream->setError(4);                     // reachedEndOfStream = 1, errorCode = 4
        Debug(DEBUGTYPE_7, "IO/BitStream.inl", 59, "")
            << "(fatal error) Value out of bounds val: " << value
            << " lower " << lower
            << " upper: " << upper;
        return;                                  // the value is NOT written
    }

    int scratch = static_cast<int>(value);
    stream->writeBits(&scratch, bitsToHold(upper + 1));
}
} // namespace

// Initializes the rank event and calls executeClient directly when local loopback
// conditions hold. Dedicated servers have no local human player for that path.
RankEvent::RankEvent(RankEventTypes type, unsigned int newRank,
                     unsigned int field14, int playerId)
    : GameEvent()
    , m_rankType(static_cast<int>(type))                     // +0x0C
    , m_newRank(newRank)                                     // +0x10
    , m_field14(field14)                                     // +0x14
    , m_playerId(playerId)                                   // +0x18
{
    if (gameLogic->isServer() && world::playerManager->getLocalHumanPlayer())   // +0x30
        executeClient();
}

int RankEvent::getType() const
{
    return GERank;
}

// Applies a promotion to player score data before notifying the HUD with the player
// pointer. Unknown event types are fatal; missing players are silently ignored.
void RankEvent::executeClient()
{
    if (m_rankType != RANK_TYPE_PROMOTION)
    {
        Debug(DEBUGTYPE_6, "Game/GameEvents/RankEvent.cpp", 102, "")
            << m_rankType << " is an unknown type.";         // aborts
        return;
    }

    world::IPlayer* player = world::playerManager->getPlayerFromId(m_playerId);   // +0x20
    if (!player)
        return;

    world::PlayerScoreData* scoreData = player->getScoreData();                   // +0x174
    scoreData->setRank(m_newRank, player);

    eventManager->postEvent(ECHud, NOTIFY_RANK, player, 0.0f);                    // +0x14
}

// executeServer - 0x80796A0, 373 B. A hard stop and nothing else - Debug type 6 aborts, so the
// return below is unreachable in the shipped build. The whole body is that one site.
// "shouldnt" is dice's spelling. Kept.
bool RankEvent::executeServer(int /*connectionId*/)
{
    Debug(DEBUGTYPE_6, "Game/GameEvents/RankEvent.cpp", 112, "")
        << "Server shouldnt get rankevents from outside.";
    return true;
}

// Decodes rank as wire value minus one, including the no-rank sentinel -1.
// The resulting rank is not range-validated.
bool RankEvent::deSerialize(io::BitStream* stream)
{
    int rankType = 0;
    stream->readBits(&rankType, bitsToHold(2));
    m_rankType = rankType;

    int newRank = 0;
    stream->readBits(&newRank, bitsToHold(32));
    m_newRank = static_cast<unsigned int>(newRank - 1);       // the -1, see above

    int field14 = 0;
    stream->readBits(&field14, 32);
    m_field14 = static_cast<unsigned int>(field14);

    int playerId = 0;
    stream->readBits(&playerId, 8);
    m_playerId = playerId;

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

// Only rank+1 is range-checked (0..31); the other fields are written unchecked.
// If the diagnostic returns after an invalid rank, that field is skipped while
// later fields are still written, leaving a short, misaligned packet.
bool RankEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int rankType = m_rankType;
    stream->writeBits(&rankType, bitsToHold(2));

    writeRangedBits(stream, m_newRank + 1u, 0u, 31u);         // the +1, and it can abort

    int field14 = static_cast<int>(m_field14);
    stream->writeBits(&field14, 32);

    int playerId = m_playerId;
    stream->writeBits(&playerId, 8);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
