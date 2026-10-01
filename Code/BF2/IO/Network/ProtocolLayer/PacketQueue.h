// Fixed-capacity receive ring with owned payload storage. capacity must be a power of two.
// [INFERRED] Header placement; the trailing flag's role remains [UNRESOLVED].

#ifndef IO_NETWORK_PROTOCOLLAYER_PACKETQUEUE_H       // [INFERRED] guard style
#define IO_NETWORK_PROTOCOLLAYER_PACKETQUEUE_H

#include "Packet.h"

namespace dice {
namespace hfe {
namespace io {

class PacketQueue
{
public:
    // Default construction preserves the existing initialization defect; use the capacity constructor.
    PacketQueue();
    explicit PacketQueue(unsigned int capacity);
    ~PacketQueue();

    void    clear();
    bool    insert(const Packet& packet);
    bool    popFront();
    Packet* getFront();

public:
    /* +0x00 */ unsigned int   m_count;      // = 0
    /* +0x04 */ unsigned int   m_tail;       // = 0
    /* +0x08 */ unsigned int   m_head;       // = 0
    /* +0x0C */ Packet*        m_packets;    // capacity records
    /* +0x10 */ unsigned char* m_payload;    // capacity * 1472 bytes
    /* +0x14 */ unsigned int   m_capacity;   // = 0 until the payload arena is allocated
    /* +0x18 */ unsigned int   m_mask;       // = capacity - 1
    /* +0x1C */ bool           m_byte1C;     // = 0  [UNRESOLVED]
};

} // namespace io
} // namespace hfe
} // namespace dice

#endif // IO_NETWORK_PROTOCOLLAYER_PACKETQUEUE_H
