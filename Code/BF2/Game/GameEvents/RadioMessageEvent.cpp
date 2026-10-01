#include "RadioMessageEvent.h"

#include "../IGame.h"                            // game->queryInterface
#include "../IGameServer.h"                      // getConnection (+0x3C), enqueueRadioMessage (+0xA4)
#include "../IGameClient.h"                      // getConnection (+0x20) - executeClient's guard
#include "../../Core/EventManager.h"             // eventManager->postEvent(cat, EventNode*, float)
#include "../../Core/Ids.h"                      // IID_IGameServer, IID_IGameClient
#include "../../IO/BitStream.h"                  // readBits / writeBits

namespace dice {
namespace hfe {

namespace {
// .rodata dword GERadioMessage (0x888A200) = 20.
const int GERadioMessage = 20;

// The two width literals. See the header block: they are the highest values OF TWO enums, not table
// sizes - TargetChannel runs TCGlobal 0 .. TCInvalid 19, and ChannelMessageType 0 .. 4 - which is
// why they are written as the widths of a value rather than as 5/3. [CHOSEN] names (file-local).
const unsigned int RADIO_CHANNEL_HIGHEST = 0x13u;   // 19 = TCInvalid -> bitsToHold gives 5
const unsigned int RADIO_TYPE_HIGHEST    = 4u;      // > bitsToHold gives 3

// The width loop both halves carry inline - see ConnectionTypeEvent.cpp for the derivation.
unsigned int bitsToHold(unsigned int highest)
{
    unsigned int bits = 0;
    do
        ++bits;
    while (((1u << bits) - 1u) < highest);
    return bits;
}

} // namespace

int RadioMessageEvent::getType() const
{
    return GERadioMessage;
}

// Posts an ECNetwork event node with sender -1 when a game client is available.
void RadioMessageEvent::executeClient()
{
    void* client = game->queryInterface(IID_IGameClient);
    if (!client)
        return;
    // A client connection is required before posting the received radio message.
    if (!static_cast<IGameClient*>(client)->getConnection())                       // +0x20
        return;

    EventNode* node = createRadioMessageReceivedEvent(
        -1,                                      // no connection on the client side
        m_messageId, m_senderId, m_team, m_field0F, m_channel, m_type);
    eventManager->postEvent(ECNetwork, node, 0.0f);                                // +0x10
}

// Requires a live connection, stamps its ID, and queues the message for server delivery.
bool RadioMessageEvent::executeServer(int connectionId)
{
    IGameServer* server = static_cast<IGameServer*>(game->queryInterface(IID_IGameServer));
    if (!server)
        return false;

    if (server->getConnection(connectionId))                                       // +0x3C
    {
        EventNode* node = createRadioMessageReceivedEvent(
            connectionId,                        // the observed sender - see the header block
            m_messageId, m_senderId, m_team, m_field0F, m_channel, m_type);
        server->enqueueRadioMessage(node);                                         // +0xA4
    }
    return true;
}

// deSerialize - 0x8078B30, 320 B. Four bytes, then the two computed-width fields.
bool RadioMessageEvent::deSerialize(io::BitStream* stream)
{
    int messageId = 0;
    stream->readBits(&messageId, 8);
    m_messageId = static_cast<unsigned char>(messageId);

    int senderId = 0;
    stream->readBits(&senderId, 8);
    m_senderId = static_cast<unsigned char>(senderId);

    int team = 0;
    stream->readBits(&team, 2);
    m_team = static_cast<unsigned char>(team);

    int field0F = 0;
    stream->readBits(&field0F, 8);
    m_field0F = static_cast<unsigned char>(field0F);

    int channel = 0;
    stream->readBits(&channel, bitsToHold(RADIO_CHANNEL_HIGHEST));                 // 5 bits
    m_channel = channel;

    int type = 0;
    stream->readBits(&type, bitsToHold(RADIO_TYPE_HIGHEST));                       // 3 bits
    m_type = type;

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

// serialize - 0x8078C70, 336 B. The exact mirror, and neither computed field is range-checked -
// unlike RankEvent's, these go out through a plain writeBits, so a channel above 19 truncates
// silently rather than aborting.
bool RadioMessageEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int messageId = m_messageId;
    stream->writeBits(&messageId, 8);

    int senderId = m_senderId;
    stream->writeBits(&senderId, 8);

    int team = m_team;
    stream->writeBits(&team, 2);

    int field0F = m_field0F;
    stream->writeBits(&field0F, 8);

    int channel = m_channel;
    stream->writeBits(&channel, bitsToHold(RADIO_CHANNEL_HIGHEST));

    int messageType = m_type;                    // `type` above is the event's own type id
    stream->writeBits(&messageType, bitsToHold(RADIO_TYPE_HIGHEST));

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

// Creates the radio notification node and initializes its payload and next pointer.
EventNode* createRadioMessageReceivedEvent(int connectionId,
                                           unsigned char messageId, unsigned char senderId,
                                           unsigned char team, unsigned char field0F,
                                           int channel, int type)
{
    return new RadioMessageReceivedEvent(connectionId, messageId, senderId, team, field0F,
                                         channel, type);
}

} // namespace hfe
} // namespace dice
