// Sliding-window connection state. [INFERRED] Header placement.
// Reference offsets do not define native STL layout. Reset permits an immediate ping; the unused time slot remains unwritten.

#ifndef IO_NETWORK_PROTOCOLLAYER_SWCONNECTION_H       // [INFERRED] guard style
#define IO_NETWORK_PROTOCOLLAYER_SWCONNECTION_H

#include <deque>

#include "../NetworkLayer/Socket/ISocket.h"           // Address, by value at +0xA8
#include "PacketQueue.h"
#include "PacketBuffer.h"

namespace dice {
namespace hfe {
namespace io {

class BitStream;

// Connection-rate telemetry. [INFERRED] Field names; the two unnamed slots remain zero.
struct CONNSTATSENTRY
{
    float bytesSentPerSecond;             // +0x00
    float bytesReceivedPerSecond;         // +0x04
    float packetsSentPerSecond;           // +0x08
    float packetsReceivedPerSecond;       // +0x0C
    float packetsLostReceivePerSecond;    // +0x10
    float packetsLostSendPerSecond;       // +0x14
    unsigned int unknown18;               // +0x18 [UNRESOLVED] type/meaning; zeroed, never accumulated
    unsigned int unknown1C;               // +0x1C [UNRESOLVED] type/meaning; zeroed, never accumulated
    unsigned int ping;                    // +0x20 - unsigned sum/division
};
static_assert(sizeof(CONNSTATSENTRY) == 0x24, "connection statistics ring stride");

class SWConnection
{
public:
    SWConnection();
    ~SWConnection();

    void reset();

    // the receive queue's two getters. Both pop one Packet and return its event code; 8 means
    // the queue was empty, 9 a zero-length payload. See the .cpp for the bounds asymmetry.
    int  getPacketPtrFromRecvQueue(unsigned char** buff, unsigned int* length);

    // The peer address, and the recv-queue push - all three
    // A connection'S address is its peer'S. m_address (+0xA8) is who this connection talks to;
    // handleConnectRequest matches an inbound datagram against it to spot a reconnect.
    const ISocket::Address& getPeerAddress() const;
    void setPeerAddress(const ISocket::Address&);
    int  addPacketToRecvQueue(Packet& packet);
    // the ID is assigned at the ACK, not at the accept. handleConnectAcceptAck is the only
    // caller: a connection carries -1 through the whole accept-sent phase and only learns its own
    // number once the client has confirmed. m_connectionId (+0xA4) is the field.
    void setConnectionId(int id);

    // Queues a data payload for send. Returns 0 when queued, 15 when full or 16 when n + 14 exceeds 1472.
    // Requires an assigned connection id; the third argument is unused.
    int  addPacketToSendBuffer(unsigned char* src, unsigned int n, unsigned int unused);

    // Sends a type-seven ping with the supplied timestamp and a prior-send flag.
    // connectionId is NetServer's outbound header value, not this connection's assigned ID.
    unsigned int sendPingRequest(BitStream* stream, unsigned char connectionId,
                                 unsigned int timeStamp);
    int  getPacketFromRecvQueue(unsigned char* dest, unsigned int* length);

    // Records delivery outcomes from the 32-packet acknowledgement window.
    // Returns false if the acknowledged count would exceed the number sent.
    bool confirmPacket(unsigned int ackSequence, unsigned int ackBitmask);
    // Pop one delivery result off the ack window (the std::deque<bool> confirmPacket fills). Returns
    // false when the window is empty; otherwise sets `delivered` to the popped bit.
    // (the GhostManager send-chain sitting) - GameServer::updateConnections drains it per connection.
    bool getPacketDeliveryStatus(bool& delivered);
    // sendPingResponse (type 8): basic+extended header, a one-bit "have I sent anything" flag
    // (0 while m_localPacketCounter is -1), then the echoed client stamp and the server stamp.
    unsigned int sendPingResponse(BitStream* stream, unsigned char connectionId,
                                  unsigned int echoTimeStamp,
                                  unsigned int serverTimeStamp);

    // Sends queued packets with current header counters.
    // Returns 0 when drained, 15 when the acknowledgement window is full, or 6 on socket refusal.
    unsigned int send(ISocket*);

    // updateStats samples per-second rates into a ring of m_state records every 500 ms - pure
    // telemetry, called by send() when the ring drains. because send() reaches it.
    // getConnectionStats now averages that ring for NetServer's status query.
    void updateStats();
    bool getConnectionStats(CONNSTATSENTRY*);

public:
    // Statistics-ring capacity; reset initializes ten records. [INFERRED] Member name.
    /* +0x000 */ unsigned int   m_state;              // reset -> 10  stats-ring capacity (see above)
    /* +0x004 */ unsigned char* m_dword4;             // = 0; delete[]'d by the destructor
    /* +0x008 */ unsigned int   m_dword08;
    /* +0x00C */ unsigned int   m_dword0C;
    /* +0x010 */ unsigned int   m_dword10;
    /* +0x014 */ unsigned int   m_dword14;
    /* +0x018 */ unsigned int   m_dword18;
    /* +0x01C */ unsigned int   m_dword1C;
    /* +0x020 */ unsigned int   m_dword20;
    /* +0x024 */ unsigned int   m_dword24;
    /* +0x028 */ unsigned int   m_dword28;
    /* +0x02C */ unsigned int   m_dword2C;            // reset -> 0
    /* +0x030 */ bool           m_byte30;             // reset -> 1   the only byte seeded true
    /* +0x034 */ unsigned int   m_dword34;            // reset -> 0
    // Tracks ownership of a reserved server slot so disconnect can return it to the pool.
    /* +0x038 */ bool           m_hasReservedSlot;    // reset -> 0
    /* +0x03C */ PacketQueue*   m_recvBuffer;         // capacity 0x100 - the receive ring
    /* +0x040 */ PacketBuffer*  m_sendBuffer;         // capacity 0x20  - the send ring
    /* +0x044 */ BitStream*     m_sendStream;         // new BitStream(0, 0)
    /* +0x048 */ unsigned int   m_dword48;            // reset -> 1
    // RE-these two are the packet counters; see the block above. Both go
    // into writeExtendedHeader, masked to six bits.
    /* +0x04C */ unsigned int   m_remotePacketCounter;  // reset -> 0
    /* +0x050 */ unsigned int   m_dword50;              // reset -> 0
    /* +0x054 */ int            m_localPacketCounter;   // reset -> -1  "nothing sent yet"
    /* +0x058 */ unsigned int   m_dword58;
    /* +0x05C */ unsigned int   m_dword5C;
    /* +0x060 */ unsigned int   m_dword60;
    /* +0x064 */ unsigned int   m_dword64;
    /* +0x068 */ unsigned int   m_dword68;
    /* +0x06C */ unsigned int   m_dword6C;
    /* +0x070 */ unsigned int   m_dword70;
    /* +0x074 */ unsigned int   m_dword74;
    /* +0x078 */ unsigned int   m_dword78;
    // Handshake timestamp in milliseconds on the server's adjusted timebase.
    /* +0x07C */ unsigned int   m_handshakeStampMs;   // reset -> 0
    /* +0x080 */ unsigned int   m_dword80;
    // Last ping timestamp includes the server clock bias; the retry check uses unadjusted elapsed time.
    /* +0x084 */ unsigned int   m_lastPingSentMs;     // reset -> 0
    /* +0x088 */ unsigned int   m_dword88;
    /* +0x08C */ unsigned int   m_dword8C;
    /* +0x090 */ unsigned int   m_dword90;
    /* +0x094 */ unsigned int   m_dword94;
    /* +0x098 */ unsigned int   m_dword98;
    /* +0x09C */ unsigned int   m_dword9C;
    // Handshake state: 0 fresh, 1 accept sent, 2 established. reset does not clear this field.
    /* +0x0A0 */ unsigned int   m_handshakeState;     // ctor -> 0
    /* +0x0A4 */ int            m_connectionId;       // ctor -> -1  unassigned
    /* +0x0A8 */ ISocket::Address m_address;          // 16 zero bytes + length 0x10 at +0xB8

    // The timestamp pair. m_time is seeded by gettimeofday in both the constructor and reset;
    // m_time2 is the eight bytes nothing recovered writes - see the header note.
    struct TimeStamp { long tv_sec; long tv_usec; };  // [INFERRED] name, matching NetServer's
    /* +0x0BC */ TimeStamp      m_time;
    /* +0x0C4 */ TimeStamp      m_time2;              // [UNRESOLVED] - never written

    /* +0x0CC */ unsigned int   m_dwordCC;            // ctor and reset -> 0
    /* +0x0D0 */ unsigned int   m_dwordD0;            // reset -> 0
    /* +0x0D4 */ unsigned int   m_dwordD4;            // reset -> 0
    // Unmasked 32-bit delay-scale value in the extended packet header.
    /* +0x0D8 */ unsigned int   m_delayScaleFactor;   // reset -> 0

    // Acknowledgment window, drained by reset. Its STL-dependent layout differs between compilers.
    /* +0x0DC */ std::deque<bool> m_ackWindow;        // [INFERRED] name - the sliding window itself

    /* +0x104 */ bool           m_byte104;            // the member the IDB was four bytes short of
};

} // namespace io
} // namespace hfe
} // namespace dice

#endif // IO_NETWORK_PROTOCOLLAYER_SWCONNECTION_H
