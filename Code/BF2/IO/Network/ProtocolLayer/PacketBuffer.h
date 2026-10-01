// Fixed-capacity send ring with owned payload storage. Rejects payloads larger than 1472 bytes.

#ifndef IO_NETWORK_PROTOCOLLAYER_PACKETBUFFER_H      // [INFERRED] guard style
#define IO_NETWORK_PROTOCOLLAYER_PACKETBUFFER_H

#include "Packet.h"

namespace dice {
namespace hfe {
namespace io {

class PacketBuffer
{
public:
    // Leaves all members uninitialized. Use the capacity constructor for a usable buffer.
    PacketBuffer();
    explicit PacketBuffer(unsigned int capacity);
    ~PacketBuffer();

    void    clear();
    Packet* getNewPacketPtr();
    bool    pushBack(const Packet& packet);
    bool    popFront();
    Packet* getFront();

public:
    /* +0x00 */ unsigned int   m_count;
    /* +0x04 */ unsigned int   m_tail;
    /* +0x08 */ unsigned int   m_head;
    /* +0x0C */ Packet*        m_packets;
    /* +0x10 */ unsigned char* m_payload;
    /* +0x14 */ unsigned int   m_capacity;
    /* +0x18 */ unsigned int   m_mask;
};

} // namespace io
} // namespace hfe
} // namespace dice

#endif // IO_NETWORK_PROTOCOLLAYER_PACKETBUFFER_H
