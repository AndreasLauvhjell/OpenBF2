// Protocol packet record shared by send and receive queues. [INFERRED] Source placement.

#ifndef IO_NETWORK_PROTOCOLLAYER_PACKET_H            // [INFERRED] guard style
#define IO_NETWORK_PROTOCOLLAYER_PACKET_H

namespace dice {
namespace hfe {
namespace io {

// Packet metadata with a borrowed payload buffer. Queue entries refer to fixed slices of their owner's arena.
struct Packet
{
    unsigned int size;                       // +0x00
    unsigned int currBytePositionBitStream;  // +0x04
    unsigned int dword8;                     // +0x08  [UNRESOLVED]
    unsigned int connectionEventType;        // +0x0C  0 = a data packet; non-zero = an event
    unsigned int dword10;                    // +0x10  [UNRESOLVED]
    const void*  pvoid14;                    // +0x14  the ring's slice - see the warning above
};

// Queue payload capacity. Server staging buffers intentionally allocate one additional byte.
const unsigned int kPacketPayloadSize = 1472;        // [INFERRED] name; the value is read

} // namespace io
} // namespace hfe
} // namespace dice

#endif // IO_NETWORK_PROTOCOLLAYER_PACKET_H
