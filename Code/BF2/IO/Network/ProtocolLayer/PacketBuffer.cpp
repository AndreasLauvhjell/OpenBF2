// Fixed packet-buffer storage for outgoing protocol data.

#include "PacketBuffer.h"

#include "../../../Core/Debug.h"
#include "../../../Core/StringUtil.h"        // formatString

#include <cstring>                           // memcpy

namespace dice {
namespace hfe {
namespace io {

// Allocates a fixed-capacity send ring and payload arena. Allocation failures emit a fatal diagnostic.
PacketBuffer::PacketBuffer(unsigned int capacity)
    : m_count(0)                                        // +0x00
    , m_tail(0)                                         // +0x04
    , m_head(0)                                         // +0x08
    , m_packets(0)                                      // +0x0C
    , m_payload(0)                                      // +0x10
    , m_capacity(0)                                     // +0x14  zero until the array lands
    , m_mask(0)                                         // +0x18
{
    m_packets = new Packet[capacity];                   // operator new[](24 * capacity)
    for (unsigned int i = 0; i < capacity; ++i)
        m_packets[i].pvoid14 = 0;

    if (!m_packets)
    {
        Debug(DEBUGTYPE_6, "IO/Network/ProtocolLayer/PacketBuffer.cpp", 36)
            << formatString("Failed to allocate memory for PacketBuffer");
        return;
    }

    m_capacity = capacity;                              // +0x14
    m_mask     = capacity - 1;                          // +0x18

    m_payload = new unsigned char[kPacketPayloadSize * capacity];
    if (!m_payload)
    {
        Debug(DEBUGTYPE_6, "IO/Network/ProtocolLayer/PacketBuffer.cpp", 50)
            << formatString("Failed to allocate memory for PacketBuffer");
        return;
    }

    unsigned int offset = 0;
    for (unsigned int i = 0; i < m_capacity; ++i)
    {
        m_packets[i].size    = 0;
        m_packets[i].dword8  = 0;
        m_packets[i].pvoid14 = m_payload + offset;
        offset += kPacketPayloadSize;
    }
}

// Constructs and destroys a temporary buffer, leaving this object uninitialized.
// Use the sized constructor; the default form preserves an existing construction defect.
PacketBuffer::PacketBuffer()
{
    // no member initialisation. That is the bug, and it is the whole function.
    PacketBuffer temp(0x40);
    (void)temp;
}

// ~PacketBuffer - ELF 0x8479560 (D1) / 0x8479500 (D2), 81 B. Arena first, records second, capacity
// zeroed; count/head/tail/mask left alone. Identical to ~PacketQueue.
PacketBuffer::~PacketBuffer()
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
    m_capacity = 0;
}

// clear - Return type [CHOSEN] void, same reasoning as PacketQueue::clear
// - Hex-Rays shows `PacketBuffer*` only because `this` is left in eax.
void PacketBuffer::clear()
{
    m_tail  = 0;
    m_head  = 0;
    m_count = 0;
}

// Claims and returns the next send slot, or null when full.
// The slot is committed before the caller fills it, so the caller must finish initializing it.
Packet* PacketBuffer::getNewPacketPtr()
{
    const unsigned int count = m_count;

    if (!count)
    {
        m_tail  = m_mask & (m_tail + 1);
        m_head  = m_tail;                               // empty: head follows tail
        m_count = count + 1;
        return &m_packets[m_tail];
    }

    if (count < m_capacity)
    {
        m_tail  = m_mask & (m_tail + 1);
        m_count = count + 1;
        return &m_packets[m_tail];
    }

    return 0;                                           // full
}

// Copies a data packet into the send ring. Returns false for a full ring or payload over 1472 bytes.
// The connection-event field is not copied.
bool PacketBuffer::pushBack(const Packet& packet)
{
    if (packet.size > kPacketPayloadSize)               // 0x5C0 - the guard, and it is first
        return false;

    Packet* const slot = getNewPacketPtr();
    if (!slot)
        return false;                                   // full

    void* const buffer = const_cast<void*>(slot->pvoid14);   // the permanent slice, preserved
    slot->dword8                    = packet.dword8;
    slot->size                      = packet.size;
    slot->currBytePositionBitStream = packet.currBytePositionBitStream;
    slot->dword10                   = packet.dword10;
    // connectionEventType is deliberately not copied - see above.

    memcpy(buffer, packet.pvoid14, packet.size);        // bounded by the check at the top
    return true;
}

// popFront -

// it zeroes only `size`. PacketQueue::popFront also zeroes dword8. Seventeen bytes of difference
// between the two functions and this is where they go; preserved rather than harmonised.
bool PacketBuffer::popFront()
{
    if (!m_count)
        return false;

    m_packets[m_head].size = 0;                         // size only - see above

    --m_count;
    m_head = m_mask & (m_head + 1);
    return true;
}

// getFront - Identical to PacketQueue::getFront.
Packet* PacketBuffer::getFront()
{
    return m_count ? &m_packets[m_head] : 0;
}

} // namespace io
} // namespace hfe
} // namespace dice
