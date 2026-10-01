// Splits and assembles network strings, publishing completed strings as events.
#include "StringManager.h"

#include <cstring>                              // memcpy - addStringDataChunk and enqueueString

#include "GameEventManager.h"                   // addEvent / getConnectionId
#include "../GameEvents/StringBlockEvent.h"     // the wire event update() emits

namespace dice {
namespace hfe {

// StringReceivedEvent - 0x808B8A0 (D1 0x808B8D0, D0 0x808B900) and 0x808B940.
// The constructor is inlined in addStringDataChunk and GameServer::say. The latter's PE/ELF
// dword stores prove targetId/flags must not narrow to the former's wire-byte inputs.
StringReceivedEvent::StringReceivedEvent(int senderId, unsigned int targetId, unsigned int flags,
                                         int channel, int type,
                                         unsigned int messageLength, unsigned char* message)
    : m_senderId(senderId)                      // +0x1C
    , m_targetId(targetId)                      // +0x20
    , m_flags(flags)                            // +0x24
    , m_channel(channel)                        // +0x28
    , m_type(type)                              // +0x2C
    , m_messageLength(messageLength)            // +0x30
    , m_message(message)                        // +0x34
{
    // EventNode() has already zeroed m_next (+0x18) - the ELF's `mov [eax+18h], 0` at 0x809F086.
}

// Owns the completed block's entire buffer, including text and trailing header bytes.
StringReceivedEvent::~StringReceivedEvent()
{
    if (m_message)
        operator delete[](m_message);
}

int StringReceivedEvent::getType() const
{
    return 11;                                  // `mov eax, 0Bh`
}

// The constructor - 0x809E0B0 (C2). As with DataBlockManager and GameEventManager, nearly all of
// its length is ONE inlined std::deque<StringDataBlock*> copy-construction from an empty temporary,
// with the same observable result: an empty deque. Written as default construction, per agents.md
// on source-level STL operations.
StringManager::StringManager(GameEventManager* gameEvents)
    : m_sending(0)                              // +0x2C
    , m_receiving(0)                            // +0x30
    , m_gameEvents(gameEvents)                  // +0x34
{
}

// the vptr re-seat and the deque's own teardown, nothing else.
// IT leaks whatever IS still queued, exactly as DataBlockManager's does: the deque holds raw
// StringDataBlock* and the destructor does not walk it. Shipped. A connection that is closing has
// nothing left to say, so it never shows.
StringManager::~StringManager()
{
}

// Queues nonempty text with a trailing twelve-byte header; sender -1 is encoded as 0.
// Requires text shorter than 244 bytes: allocation size wraps in one byte while
// the copy uses the full length, and this function performs no length check.
bool StringManager::enqueueString(TargetChannel channel, ChannelMessageType type, int senderId,
                                  const std::string& text,
                                  int unknown9, int flags)
{
    const unsigned int length = static_cast<unsigned int>(text.length());
    if (!length)
        return false;                           // empty strings never reach the wire

    StringDataBlock* const block = new StringDataBlock;
    block->m_size     = 0;
    block->m_position = 0;
    block->m_data     = 0;
    block->m_header   = 0;

    block->m_size     = static_cast<unsigned char>(
                            static_cast<unsigned char>(length) + 12);   // byte arithmetic - see above
    block->m_data     = new unsigned char[block->m_size];
    block->m_position = 0;
    block->m_header   = block->m_data + length;                         // header trails the text

    StringDataHeader* const header = reinterpret_cast<StringDataHeader*>(block->m_header);
    header->m_channel  = static_cast<int>(channel);
    header->m_type     = static_cast<int>(type);
    header->m_senderId = (senderId == -1) ? 0 : static_cast<unsigned char>(senderId);  // 1 -> 0
    header->m_unknown9 = static_cast<unsigned char>(unknown9);
    header->m_flags    = static_cast<unsigned char>(flags);

    std::memcpy(block->m_data, text.c_str(), length);   // full dword length - see above

    m_pending.push_back(block);
    return true;
}

// Sends a size announcement followed by up to three 63-byte chunks per tick.
// Retains progress between ticks and frees both buffer and block when complete.
// Received buffers instead transfer to StringReceivedEvent.
void StringManager::update()
{
    if (!m_sending)
    {
        if (m_pending.empty())
            return;
        m_sending = m_pending.front();
        m_pending.pop_front();
    }
    if (!m_sending)
        return;

    if (m_sending->m_position == 0)
    {
        StringBlockEvent* const announce = new StringBlockEvent(m_sending->m_size);
        m_gameEvents->addEvent(announce);
    }

    int budget = 2;
    while (m_sending->m_position < m_sending->m_size)
    {
        unsigned int chunk = static_cast<unsigned int>(m_sending->m_size)
                           - static_cast<unsigned int>(m_sending->m_position);
        if (chunk >= 63)
            chunk = 63;

        StringBlockEvent* const payload =
            new StringBlockEvent(m_sending->m_data + m_sending->m_position,
                                 static_cast<unsigned char>(chunk));
        m_gameEvents->addEvent(payload);

        m_sending->m_position = static_cast<unsigned char>(m_sending->m_position + chunk);

        if (m_sending->m_position >= m_sending->m_size)
            break;
        if (--budget == -1)
            return;                             // resume next tick - see above
    }

    if (m_sending->m_data)
        operator delete[](m_sending->m_data);
    delete m_sending;                           // the sender frees both - see above
    m_sending = 0;
}

// Starts one incoming string block, refusing a second while assembly is active.
// Size truncates to a byte before allocation, keeping storage and recorded size consistent.
bool StringManager::newStringDataBlock(unsigned int size)
{
    if (m_receiving)
        return false;                           // already assembling one

    StringDataBlock* const block = new StringDataBlock;
    block->m_size     = 0;
    block->m_position = 0;
    block->m_data     = 0;
    block->m_header   = 0;

    block->m_size     = static_cast<unsigned char>(size);
    block->m_data     = new unsigned char[block->m_size];
    block->m_position = 0;
    block->m_header   = block->m_data + size - 12;

    m_receiving = block;
    return true;
}

// Appends a string chunk, rejecting announced-size overruns. Completion transfers the buffer to an event.
// Untrusted senders are replaced by the connection ID; non-chat types and server-only channels
// are silently dropped while returning true. Those suppressed-message paths retain the original buffer leak.
bool StringManager::addStringDataChunk(void* chunk, int length, bool trustSenderId)
{
    if (!m_receiving)
        return false;

    if (static_cast<unsigned int>(length) + m_receiving->m_position > m_receiving->m_size)
        return false;                           // fails closed - see above

    std::memcpy(m_receiving->m_data + m_receiving->m_position, chunk, length);
    m_receiving->m_position =
        static_cast<unsigned char>(m_receiving->m_position + static_cast<unsigned char>(length));

    if (m_receiving->m_position != m_receiving->m_size)
        return true;                            // not complete yet

    const StringDataHeader* const header =
        reinterpret_cast<const StringDataHeader*>(m_receiving->m_header);

    int  senderId = header->m_senderId;
    bool post     = true;

    if (!trustSenderId)                         // Server-side sender validation.
    {
        senderId = m_gameEvents->getConnectionId();
        if (header->m_type != 2 || header->m_channel == 12 || header->m_channel == 13)
            post = false;                       // silent drop, and the buffer leaks with it
    }

    if (post)
    {
        StringReceivedEvent* const received =
            new StringReceivedEvent(senderId,
                                    header->m_unknown9,
                                    header->m_flags,
                                    header->m_channel,
                                    header->m_type,
                                    static_cast<unsigned int>(m_receiving->m_size) - 12,
                                    m_receiving->m_data);
        eventManager->postEvent(ECNetwork, received, 0.0f);
    }

    delete m_receiving;                         // the block only - see above
    m_receiving = 0;
    return true;
}

} // namespace hfe
} // namespace dice
