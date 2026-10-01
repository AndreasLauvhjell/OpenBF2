// Reliable, ordered GameEvent transport (stream-manager ID 1).
// [INFERRED] Header path; recovered from the dedicated ELF.
// Each direction uses a 32-block window. A duplicate pending block is an error.
// reset() clears the block cursors but preserves m_lastConfirmedBlockIndex.

#ifndef GAME_COMMON_GAMEEVENTMANAGER_H        // [INFERRED] guard style
#define GAME_COMMON_GAMEEVENTMANAGER_H

#include <deque>
#include <list>
#include <map>

#include "IStreamManager.h"


#include "../../Core/SmartPtr.h"

namespace dice {
namespace hfe {

class GameEvent;

// Reference-counted events retained under their send-block index for delivery tracking.
struct EventSaveQueueEntry
{
    /* +0x00 */ int                              blockIndex;
    /* +0x04 */ std::list<SmartPtr<GameEvent> >  events;
};

class GameEventManager : public IStreamManager
{
public:
    // both RW-24's lifecycle half is retired. The five wire bodies
    // (transmit, handlePacketStatus, writeGameEvent, readGameEvent, insertEvents) remain.
    explicit GameEventManager(bool isDemoPlayback);
    virtual ~GameEventManager();

    // IBase
    virtual void  addRef();                                      // +0x00
    virtual int   getRef();                                      // +0x04
    virtual int   release();                                     // +0x08
    virtual void* queryInterface(unsigned int iid) const;        // +0x0C

    // IStreamManager
    virtual int   getStreamManagerId() const;                    // +0x10
    virtual void  setStreamManagerId(int id);                    // +0x14
    virtual bool  processReceivedPacket(io::BitStream* stream);  // +0x18
    virtual void  handlePacketStatus(PacketStatus& status, bool& delivered);
                                                                 // +0x1C  [FRONTIER]
    // return type see IStreamManager.h. A caller does
    // `transmitResult >> 3` on it; this is the BIT count written.
    virtual unsigned int transmit(io::BitStream* stream, PacketStatus& status,
                                  unsigned int budget);
                                                                 // +0x20  [FRONTIER]
    virtual void  setConnectionId(int connectionId);             // +0x24
    virtual int   getConnectionId();                             // +0x28
    virtual int   getError();                                    // +0x2C

    // its OWN four virtuals, appended after the destructor pair
    virtual void  reset();                                       // +0x38
    virtual bool  addEvent(GameEvent* event);                    // +0x3C
    virtual GameEvent* getNextRcvdEvent();                       // +0x40
    virtual bool  addEventToSendQueue(GameEvent* event);         // +0x44

    // NON-virtual
    int  getNumUnconfirmed();
    bool advanceLastConfirmedBlockIndex();

    // ALL three
    // return type bool. The ELF materialises 0 on both failure arms and 1 on
    // success, and transmit consumes it (`if (!writeGameEvent(...)) break;`) to decide when the
    // packet is full - so this is a caller-confirmed return, not an inference.
    bool writeGameEvent(io::BitStream* stream, const SmartPtr<GameEvent>& event);
    GameEvent* readGameEvent(io::BitStream* stream);
    bool insertEvents(std::list<SmartPtr<GameEvent> >* block);

public:
    // The +4/+8 read/write counters are inherited from IStreamManager.
    /* +0x00C */ int  m_refCount;
    /* +0x010 */ int  m_connectionId;
    /* +0x014 */ int  m_error;
    /* +0x018 */ std::list<SmartPtr<GameEvent> > m_sendQueue;
    // Delivered batches enter m_sentBlocks and advance the confirmed cursor; failed
    // batches move to the resend queue. Either outcome removes the in-flight front.
    // [INFERRED] member names.
    /* +0x01C */ std::deque<EventSaveQueueEntry> m_inFlightBlocks;   // sent, awaiting confirmation
    /* +0x044 */ std::deque<EventSaveQueueEntry> m_resendBlocks;     // lost, waiting to go again
    /* +0x06C */ std::map<int, std::list<SmartPtr<GameEvent> >*> m_pendingRecvBlocks;
    /* +0x078 */ std::list<SmartPtr<GameEvent> > m_recvEvents;
    /* +0x07C */ int  m_nextSendBlockIndex;
    /* +0x080 */ int  m_nextRecvBlockIndex;
    /* +0x084 */ int  m_lastConfirmedBlockIndex;   // reset() does NOT clear it. ctor -> 31
    /* +0x088 */ int  m_dword88;                   // ctor -> 15  [UNRESOLVED name]
    /* +0x08C */ int  m_streamManagerId;
    /* +0x090 */ int  m_sendSequence;
    /* +0x094 */ int  m_dword94;                   // ctor -> 0   [UNRESOLVED name]
    /* +0x098 */ int  m_recvEventCount;
    // the receive sequence counter, the mirror of m_sendSequence (+0x090).
    // insertEvents stamps every arriving event's +0x08 with it and post-increments.
    /* +0x09C */ int  m_recvSequence;              // ctor -> 0
    /* +0x0A0 */ bool m_byteA0;                    // ctor -> 0   [UNRESOLVED name]
    /* +0x0A4 */ std::list<SmartPtr<GameEvent> >* m_sentBlocks[32];
    /* +0x124 */ bool m_isDemoPlayback;
};

#if defined(_WIN64) || defined(__LP64__)
// [CHOSEN, not recovered] Native IStreamManager prefix; STL members have native layout.
// Connection IDs, errors and event serialization retain their existing 32-bit types.
static_assert(offsetof(GameEventManager, m_refCount) == 16, "native x64 GameEventManager prefix");
static_assert(offsetof(GameEventManager, m_connectionId) == 20, "native x64 GameEventManager connection ID");
static_assert(offsetof(GameEventManager, m_error) == 24, "native x64 GameEventManager error");
#else
static_assert(offsetof(GameEventManager, m_refCount) == 12, "retail GameEventManager prefix");
static_assert(offsetof(GameEventManager, m_connectionId) == 16, "retail GameEventManager connection ID");
static_assert(offsetof(GameEventManager, m_error) == 20, "retail GameEventManager error");
#endif

} // namespace hfe
} // namespace dice

#endif // GAME_COMMON_GAMEEVENTMANAGER_H
