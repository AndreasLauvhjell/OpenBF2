// Fixed packet-queue storage for received protocol data.

#include "PacketQueue.h"

#include "../../../Core/Debug.h"
#include "../../../Core/StringUtil.h"        // formatString

#include <cstring>                           // memcpy

namespace dice {
namespace hfe {
namespace io {

// Allocates a receive ring and payload arena. capacity must be a power of two; it is not checked.
// Allocation failures emit a fatal diagnostic.
PacketQueue::PacketQueue(unsigned int capacity)
    : m_count(0)                                        // +0x00
    , m_tail(0)                                         // +0x04
    , m_head(0)                                         // +0x08
    , m_packets(0)                                      // +0x0C - assigned below
    , m_payload(0)                                      // +0x10
    , m_capacity(0)                                     // +0x14  zero until the array lands
    , m_mask(0)                                         // +0x18
    , m_byte1C(false)                                   // +0x1C
{
    m_packets = new Packet[capacity];                   // operator new[](24 * capacity)
    for (unsigned int i = 0; i < capacity; ++i)
        m_packets[i].pvoid14 = 0;                       // the `*(p + 20) = 0` stride walk

    if (!m_packets)
    {
        Debug(DEBUGTYPE_6, "IO/Network/ProtocolLayer/PacketQueue.cpp", 34)
            << formatString("Failed to allocate memory for PacketQueue");
        return;
    }

    m_capacity = capacity;                              // +0x14
    m_mask     = capacity - 1;                          // +0x18  the ring mask

    m_payload = new unsigned char[kPacketPayloadSize * capacity];
    if (!m_payload)
    {
        Debug(DEBUGTYPE_6, "IO/Network/ProtocolLayer/PacketQueue.cpp", 48)
            << formatString("Failed to allocate memory for PacketQueue");
        return;
    }

    unsigned int offset = 0;
    for (unsigned int i = 0; i < m_capacity; ++i)
    {
        m_packets[i].size    = 0;
        m_packets[i].dword8  = 0;
        m_packets[i].pvoid14 = m_payload + offset;      // the permanent slice
        offset += kPacketPayloadSize;
    }

    m_byte1C = false;                                   // +0x1C, last
}

// Constructs and destroys a temporary queue, leaving this object uninitialized except for its trailing flag.
// Use the sized constructor; the default form preserves an existing construction defect.
PacketQueue::PacketQueue()
{
    m_byte1C = false;                                   // +0x1C, the only member touched

    PacketQueue temp(0x40);                             // built and thrown away - see above
    (void)temp;
}

// Frees the payload arena before the packet records.
PacketQueue::~PacketQueue()
{
    if (m_payload)
    {
        delete[] m_payload;
        m_payload = 0;
    }
    if (m_packets)
    {
        delete[] m_packets;
        m_packets = 0;
    }
    m_capacity = 0;                                     // only the capacity is reset
}

// Clears queue cursors without overwriting records or payloads.
void PacketQueue::clear()
{
    m_tail  = 0;                                        // +0x04
    m_head  = 0;                                        // +0x08
    m_count = 0;                                        // +0x00
}

// Enqueues a packet, returning false when full. Data payloads must be at most 1472 bytes; no size check is performed.
// Event packets copy only the event type and leave the previous payload fields intact.
bool PacketQueue::insert(const Packet& packet)
{
    const unsigned int count = m_count;

    if (count)
    {
        if (count >= m_capacity)
            return false;                               // full - the only refusal
        m_tail = m_mask & (m_tail + 1);
    }
    else
    {
        m_tail = m_mask & (m_tail + 1);
        m_head = m_tail;                                // see the note above
    }
    m_count = count + 1;

    Packet& slot = m_packets[m_tail];
    slot.connectionEventType = packet.connectionEventType;      // always

    if (packet.connectionEventType == 0)                        // data packets only
    {
        void* const buffer = const_cast<void*>(slot.pvoid14);   // the permanent slice, kept
        slot.dword8                   = packet.dword8;
        slot.size                     = packet.size;
        slot.currBytePositionBitStream = packet.currBytePositionBitStream;
        slot.dword10                  = packet.dword10;

        // unbounded. See the block comment above. packet.size is trusted here and the slice is
        // 1472 bytes. PacketBuffer::pushBack rejects `size > 0x5C0` before doing the same thing;
        // this one does not, and the difference is reproduced rather than harmonised.
        memcpy(buffer, packet.pvoid14, packet.size);
    }
    return true;
}

// popFront -

// it zeroes two fields of the outgoing slot, size and dword8 - PacketBuffer::popFront zeroes only
// size. Another asymmetry between the two classes, preserved.
bool PacketQueue::popFront()
{
    if (!m_count)
        return false;

    m_packets[m_head].size   = 0;
    m_packets[m_head].dword8 = 0;                       // PacketBuffer does not do this one

    --m_count;
    m_head = m_mask & (m_head + 1);
    return true;
}

// getFront - Null when empty; no bounds concern because m_head is masked.
Packet* PacketQueue::getFront()
{
    return m_count ? &m_packets[m_head] : 0;
}

} // namespace io
} // namespace hfe
} // namespace dice
