// One packet substream owned by ClientConnection. Registration order is a wire
// contract: player actions, game events, then ghosts. Packet-status masks use these
// indices. The interface has no virtual destructor; release through IBase.
// Read/write bit counters belong to the base; connection and stream ids belong
// to implementers. RW-20 still tracks outputStreamManagerStats.

#ifndef GAME_COMMON_ISTREAMMANAGER_H          // [INFERRED] guard style
#define GAME_COMMON_ISTREAMMANAGER_H

#include "../../Core/IBase.h"
#include <cstddef>

namespace dice {
namespace hfe {

namespace io { class BitStream; }
struct PacketStatus;                          // 48 bytes - Game/Common/ClientConnection.h

class IStreamManager : public IBase
{
public:
    // [INFERRED] source-level accessor/member names; these are the inlined signed loads and stores
    // in the paired samplers. They are nonvirtual and do not add slots. No constructor initializes
    // either counter in the three retail implementers; codec writes replace the last packet count
    // and the sampler resets it to zero after reading it.
    int getReadBits() const { return m_readBits; }
    int getWriteBits() const { return m_writeBits; }
    void setReadBits(int bits) { m_readBits = bits; }
    void setWriteBits(int bits) { m_writeBits = bits; }

    int m_readBits;  // +0x04, signed cvtsi2ss (ELF) / fild dword (PE)
    int m_writeBits; // +0x08

    // Preserve virtual order. The stream ID is write-once: later assignments retain
    // the first registered index.
    virtual int  getStreamManagerId() const                                       = 0;  // +0x10
    virtual void setStreamManagerId(int id)                                       = 0;  // +0x14

    // returns bool - ClientConnection::processReceivedPacket ANDs the answers of all three
    // together and hands the result back to GameServer, where a false disconnects with reason 29.
    virtual bool processReceivedPacket(io::BitStream* stream)                     = 0;  // +0x18

    // PlayerActionManager's IS five bytes - AN empty body. Player actions are fire-and-forget:
    // they do not care whether the packet carrying them arrived. GameEventManager's is 608 bytes
    // and GhostManager's 912, because both have to re-queue what was lost.
    virtual void handlePacketStatus(PacketStatus& status, bool& delivered)        = 0;  // +0x1C

    // Appends this manager's payload and returns the number of bits written.
    virtual unsigned int transmit(io::BitStream* stream, PacketStatus& status,
                          unsigned int budget)                                    = 0;  // +0x20

    virtual void setConnectionId(int connectionId)                                = 0;  // +0x24
    virtual int  getConnectionId()                                                = 0;  // +0x28

    // GhostManager error 2 means malformed received data and remains latched.
    // [UNRESOLVED] Other error values.
    virtual int  getError()                                                       = 0;  // +0x2C

protected:
    // NON-virtual AND protected, like ~IBase and ~ISquadManager, and for the same reason: the
    // implementers' destructor pairs sit at +0x30/+0x34, after these eight slots, which is only
    // consistent with this interface not declaring one. See the table above.
    ~IStreamManager() {}
};

#if defined(_WIN64) || defined(__LP64__)
// [CHOSEN, not recovered] Native vptr followed by the same two signed 32-bit counters. The Linux
// x86-64 port (__LP64__) shares every native 64-bit contract in this tree
// that involves no MSVC-STL width; the retail x86 branch below is what the 32-bit builds keep.
// Consumers use these members/accessors; the x86 offsets are evidence, not wire offsets.
static_assert(sizeof(IStreamManager) == 16, "native x64 stream-manager prefix");
static_assert(offsetof(IStreamManager, m_readBits) == 8, "native x64 read-counter offset");
static_assert(offsetof(IStreamManager, m_writeBits) == 12, "native x64 write-counter offset");
#else
static_assert(sizeof(IStreamManager) == 12, "x86 IStreamManager contains a vptr and two counters");
static_assert(offsetof(IStreamManager, m_readBits) == 4, "retail read-counter offset");
static_assert(offsetof(IStreamManager, m_writeBits) == 8, "retail write-counter offset");
#endif
static_assert(sizeof(IStreamManager::m_readBits) == 4 && sizeof(IStreamManager::m_writeBits) == 4,
              "stream bit counters remain signed 32-bit values");

} // namespace hfe
} // namespace dice

#endif // GAME_COMMON_ISTREAMMANAGER_H
