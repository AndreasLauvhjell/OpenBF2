// Reliable event framing and delivery state. [INFERRED] source path.

#include "GameEventManager.h"

#include "GameEvent.h"
#include "../../Core/StringUtil.h"           // formatString - the resetWritePosition assert
#include "ClientConnection.h"           // PacketStatus - transmit fills three of its fields
#include "../../Core/Ids.h"                      // IID_IBase, IID_IStreamManager
#include "../../IO/BitStream.h"
#include "../../Core/Debug.h"             // the MaximumGameEventSize warning

namespace dice {
namespace hfe {

// Initializes event queues and a 32-slot block ring with a fifteen-block send window.
// The confirmed cursor starts immediately before block zero; stream-manager ID starts unset.
// Inherited counters and connection ID remain uninitialized until their owning paths assign them.
GameEventManager::GameEventManager(bool isDemoPlayback)
    : m_refCount(1)                         // +0x00C
    , m_error(0)                            // +0x014
    , m_nextSendBlockIndex(0)               // +0x07C
    , m_nextRecvBlockIndex(0)               // +0x080
    , m_lastConfirmedBlockIndex(31)         // +0x084  31, not 0 - see above
    , m_dword88(15)                         // +0x088  15
    , m_streamManagerId(-1)                 // +0x08C  the write-once sentinel
    , m_sendSequence(0)                     // +0x090
    , m_dword94(0)                          // +0x094
    , m_recvEventCount(0)                   // +0x098
    , m_recvSequence(0)                          // +0x09C
    , m_byteA0(false)                       // +0x0A0
    , m_isDemoPlayback(isDemoPlayback)      // +0x124
{
    // m_readBits (+0x004) and m_connectionId (+0x010) are NOT set here - see the block
    // above. m_sendQueue, both deques, m_pendingRecvBlocks and m_recvEvents default-construct.

    for (int i = 0; i <= 31; ++i)           // +0x0A4..+0x123, the last thing the ELF does
        m_sentBlocks[i] = 0;
}

// Releases queued events and send blocks before member containers are destroyed.
GameEventManager::~GameEventManager()
{
    reset();
    // Members are destroyed here by the compiler, in reverse declaration order - see above.
}

// Drains a completed block into the receive queue in order, assigning each event
// the next receive sequence number.
bool GameEventManager::insertEvents(std::list<SmartPtr<GameEvent> >* block)
{
    while (!block->empty())                             // the ELF counts to decide this
    {
        SmartPtr<GameEvent> event = block->front();
        block->erase(block->begin());

        event->m_sendSequence = m_recvSequence;              // GameEvent +0x08 <- manager +0x9C
        ++m_recvSequence;

        m_recvEvents.insert(m_recvEvents.end(), event);
    }
    return true;
}

// Reads one event using the registry's current type-id width. Client and server
// registries must agree. Returns null for an unknown id or failed decode; a successful
// result carries one reference owned by the caller.
GameEvent* GameEventManager::readGameEvent(io::BitStream* stream)
{
    unsigned int bits = 0;
    const unsigned int highest = gameEventRegistry->getHighestEventId();
    if (highest)
    {
        do
            ++bits;
        while (highest > (1u << bits) - 1u);
    }

    unsigned int id = 0;
    stream->readBits(&id, bits);

    GameEvent* event = 0;
    IGameEventMaker* const maker = gameEventRegistry->getEventMaker(id);
    if (maker)
        event = maker->createEvent();          // IGameEventMaker's only slot
    if (!event)
        return 0;

    GameEvent* result = 0;
    if (event->deSerialize(stream))
    {
        result = event;
        event->addRef();                                // hand one reference to the caller
    }
    event->release();
    return result;
}

// Serializes one event, restoring position, high-water mark, and error state if it
// does not fit. The 1024-bit maximum is checked only after successful serialization;
// that failure returns false without undoing the oversized event.
bool GameEventManager::writeGameEvent(io::BitStream* stream, const SmartPtr<GameEvent>& event)
{
    const unsigned int startBits = stream->m_highWaterBits;

    // take the rollback mark, in the stream's own slots
    stream->m_byte38 = true;
    stream->m_positionCache = (stream->m_readWriteSwitch == 1) ? stream->m_readBitPos
                                                               : stream->m_bitPos;
    stream->m_rwSwitchCache = stream->m_readWriteSwitch;
    stream->m_lengthCache   = stream->m_highWaterBits;

    if (!event->serialize(stream))
    {
        stream->setPosition(*reinterpret_cast<io::BSPosition*>(&stream->m_dword3C));
        stream->m_byte38        = false;
        stream->m_errorCode     = 0;
        stream->m_error         = false;
        stream->m_highWaterBits = stream->m_lengthCache;
        return false;
    }
    stream->m_byte38 = false;

    event->logInfo(true, m_connectionId);

    const unsigned int bitSize = stream->m_highWaterBits - startBits;
    if (bitSize > 1024)                                 // MaximumGameEventSize, in bits
    {
        Debug(DEBUGTYPE_7, "Game/Common/GameEventManager.cpp", 539)
            << "bitSize > (uint)MaximumGameEventSize"
            << "Event of type " << event->getType() << " size is too big" << bitSize;
        return false;                                   // the bits stay written - see above
    }
    return true;
}

// Resolves the oldest in-flight block; transport notifications must arrive in send
// order because status is ignored. Delivered blocks advance the confirmation cursor;
// lost nonempty blocks are queued for resend. Empty lost blocks are discarded.
void GameEventManager::handlePacketStatus(PacketStatus& /*status*/, bool& delivered)
{
    std::list<SmartPtr<GameEvent> > events;             // the local the ELF allocates up front

    if (m_inFlightBlocks.empty())
        return;

    const int blockIndex = m_inFlightBlocks.front().blockIndex;
    events = m_inFlightBlocks.front().events;

    if (delivered)
    {
        std::list<SmartPtr<GameEvent> >* const confirmed =
            new std::list<SmartPtr<GameEvent> >();      // operator new(4) + its sentinel

        for (std::list<SmartPtr<GameEvent> >::iterator it = events.begin();
             it != events.end(); ++it)
            confirmed->insert(confirmed->end(), *it);

        m_sentBlocks[blockIndex] = confirmed;           // +0xA4 + blockIndex*4

        while (advanceLastConfirmedBlockIndex())
            ;                                           // greedy - see above
    }
    else if (!events.empty())                           // an empty lost block is NOT resent
    {
        EventSaveQueueEntry entry;
        entry.blockIndex = blockIndex;
        entry.events     = events;
        m_resendBlocks.push_back(entry);
    }

    m_inFlightBlocks.pop_front();
}

// Writes queued events within the optional bit budget, returning bits consumed.
// Resends take precedence and must fit as a whole; a failed resend sets m_error and returns zero.
// Fresh batches stop when the next event does not fit and wait when the unconfirmed window is full.
// An empty batch writes one absence bit. The temporary stream limit is restored on every exit.
unsigned int GameEventManager::transmit(io::BitStream* stream, PacketStatus& status,
                                        unsigned int budget)
{
    unsigned int written  = 0;
    bool         isResend = false;
    std::list<SmartPtr<GameEvent> > pending;

    const unsigned int startBits    = stream->m_highWaterBits;
    const unsigned int savedLimit   = stream->m_bitLimit;          // +0x08, "packet size" in bits
    if (budget)
    {
        unsigned int cap = budget + stream->m_highWaterBits;
        if (stream->m_bitLimit <= cap)
            cap = stream->m_bitLimit;
        stream->m_bitLimit = cap;
    }

    // the mark the header patch-back seeks to
    io::BSPosition mark;
    if (stream->m_readWriteSwitch == 1)
    {
        mark.m_readWriteSwitch = 1;                                // the literal, not a re-read
        mark.m_position        = stream->m_readBitPos;
    }
    else
    {
        mark.m_readWriteSwitch = stream->m_readWriteSwitch;
        mark.m_position        = stream->m_bitPos;
    }
    stream->m_byte38        = true;
    stream->m_positionCache = mark.m_position;
    stream->m_rwSwitchCache = stream->m_readWriteSwitch;
    stream->m_lengthCache   = stream->m_highWaterBits;

    if (!stream->skipBits(15))                                     // no room even for the header
    {
        stream->setPosition(*reinterpret_cast<io::BSPosition*>(&stream->m_dword3C));
        stream->m_byte38        = false;
        stream->m_errorCode     = 0;
        stream->m_error         = false;
        stream->m_highWaterBits = stream->m_lengthCache;
        if (budget)
            stream->m_bitLimit = savedLimit;
        unsigned int nothing = 0;
        stream->writeBits(&nothing, 1);
        return 1;                                                  // 1, not 0 - see above
    }
    stream->m_byte38 = false;

    unsigned int blockIndex = 0;

    if (m_resendBlocks.empty())
    {
        // fresh: drain m_sendQueue until it is empty, full, or 255 events
        if (!m_sendQueue.empty() && getNumUnconfirmed() < m_dword88)
        {
            std::list<SmartPtr<GameEvent> > batch;
            while (!m_sendQueue.empty())
            {
                SmartPtr<GameEvent> event = m_sendQueue.front();
                if (!writeGameEvent(stream, event))
                    break;                                         // soft stop - the rest waits
                m_sendQueue.erase(m_sendQueue.begin());
                batch.insert(batch.end(), event);
                ++m_dword94;
                ++written;
                if (written > 254)                                 // the 8-bit count
                    break;
            }
            if (written)
            {
                blockIndex          = m_nextSendBlockIndex & 31;
                status.m_eventCount = written;
                status.m_blockIndex = blockIndex;
                pending             = batch;

                EventSaveQueueEntry entry;
                entry.blockIndex = static_cast<int>(blockIndex);
                entry.events     = pending;
                m_inFlightBlocks.push_back(entry);

                ++m_nextSendBlockIndex;                            // only the fresh path advances it
            }
        }
    }
    else
    {
        // resend: the oldest lost block, whole or not at all
        blockIndex = static_cast<unsigned int>(m_resendBlocks.front().blockIndex);
        pending    = m_resendBlocks.front().events;
        m_resendBlocks.pop_front();

        std::list<SmartPtr<GameEvent> > batch;
        while (!pending.empty())
        {
            SmartPtr<GameEvent> event = pending.front();
            if (!writeGameEvent(stream, event))
            {
                m_error = 1;                                       // hard failure - see above
                return 0;
            }
            pending.erase(pending.begin());
            batch.insert(batch.end(), event);
            ++written;
            isResend = true;
        }
        if (written)
        {
            status.m_eventCount = written;
            status.m_blockIndex = blockIndex;
            pending             = batch;

            EventSaveQueueEntry entry;
            entry.blockIndex = static_cast<int>(blockIndex);
            entry.events     = pending;
            m_inFlightBlocks.push_back(entry);
        }
    }

    if (written)
    {
        status.m_streamManagerMask |= 1u << getStreamManagerId();

        stream->setWritePosition(mark);                            // back to the reserved hole
        unsigned int one = 1;
        stream->writeBits(&one, 1);
        unsigned int count = status.m_eventCount;
        stream->writeBits(&count, 8);
        unsigned int block = status.m_blockIndex;
        stream->writeBits(&block, 5);
        unsigned int resendBit = isResend ? 1u : 0u;
        stream->writeBits(&resendBit, 1);

        if (stream->m_canResetWritePosition)                       // +0x50
        {
            stream->m_canResetWritePosition = false;
            stream->setPosition(*reinterpret_cast<io::BSPosition*>(&stream->m_dword24));
        }
        else
        {
            Debug(DEBUGTYPE_6, "IO/BitStream.inl", 341)
                << formatString("Call to resetWritePosition without prior call to setPosition");
        }

        m_sentBlocks[status.m_blockIndex] = 0;                     // this block is in flight again
    }
    else
    {
        stream->setPosition(mark);
        stream->setBitSize(startBits);
        unsigned int nothing = 0;
        stream->writeBits(&nothing, 1);
    }

    if (budget)
        stream->m_bitLimit = savedLimit;

    const unsigned int used = stream->m_highWaterBits - startBits;
    m_writeBits = used;                                         // +0x08
    return used;
}

void GameEventManager::addRef() { ++m_refCount; }
int  GameEventManager::getRef() { return m_refCount; }

int GameEventManager::release()
{
    if (--m_refCount == 0)
    {
        delete this;
        return 0;
    }
    return m_refCount;
}

void* GameEventManager::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase)
        return const_cast<GameEventManager*>(this);
    if (iid == IID_IStreamManager)
        return const_cast<GameEventManager*>(this);
    return 0;
}

// The IStreamManager accessors - 0x8090F70..0x8090FB0. setStreamManagerId is the write-once form
// IStreamManager.h describes; its 28 bytes against PlayerActionManager's 24 are the two extra bytes
// the wider offset costs.
int  GameEventManager::getStreamManagerId() const { return m_streamManagerId; }   // +0x08C
int  GameEventManager::getConnectionId()          { return m_connectionId; }      // +0x010
int  GameEventManager::getError()                 { return m_error; }             // +0x014
void GameEventManager::setConnectionId(int id)    { m_connectionId = id; }        // +0x010

void GameEventManager::setStreamManagerId(int id)
{
    if (m_streamManagerId == -1)                         //   write-once
        m_streamManagerId = id;
}

// Consumes the caller's event reference. Demo playback releases it and returns true;
// other connections dispatch virtually to addEventToSendQueue.
bool GameEventManager::addEvent(GameEvent* event)
{
    if (!m_isDemoPlayback)                               //   byte [this+0x124]
        return addEventToSendQueue(event);               // virtual, +0x44

    if (event)
        event->release();                                // swallowed
    return true;
}

// Consumes the caller's event reference and queues it in chronological sequence order.
// Returns false for null. The queue retains its own reference until transmission/retirement.
bool GameEventManager::addEventToSendQueue(GameEvent* event)
{
    if (!event)
        return false;

    event->addRef();
    event->m_sendSequence = m_sendSequence;              // the event's +0x08
    ++m_sendSequence;

    m_sendQueue.insert(m_sendQueue.end(), SmartPtr<GameEvent>(event));     // push_back - chronological (ELF 0x808E930)

    event->release();                                    // the consume
    event->release();                                    // the temporary
    return true;
}

// Pops the next received event, transferring one reference to the caller, or returns
// null when empty. Keeps the size() emptiness check used by the original implementation.
GameEvent* GameEventManager::getNextRcvdEvent()
{
    if (m_recvEvents.size() == 0)                        // size(), not empty() - see above
        return 0;

    GameEvent* const event = m_recvEvents.front();
    if (event)
        event->addRef();                                 // the caller's reference
    m_recvEvents.erase(m_recvEvents.begin());
    return event;
}

// Returns modulo-32 cursor distance. Full and empty windows both report zero;
// callers must distinguish them from other state.
int GameEventManager::getNumUnconfirmed()
{
    return ((m_nextSendBlockIndex & 31) - m_lastConfirmedBlockIndex) & 31;
}

// Acknowledges at most one sent block without overtaking the write cursor.
// Notifies each retained event before releasing it, passing the connection ID by mutable address.
// Returns false when no next block can be retired.
bool GameEventManager::advanceLastConfirmedBlockIndex()
{
    const int next = (m_lastConfirmedBlockIndex + 1) & 31;
    if (next == (m_nextSendBlockIndex & 31))             //   never overtake
        return false;

    std::list<SmartPtr<GameEvent> >* const block = m_sentBlocks[next];
    if (!block)
        return false;

    while (!block->empty())
    {
        GameEvent* const event = block->front();
        if (event)
            event->addRef();
        event->eventReceivedByRemote(&m_connectionId);   // GameEvent +0x20, BY address
        block->erase(block->begin());
        if (event)
            event->release();
    }

    delete block;
    m_sentBlocks[next] = 0;
    m_lastConfirmedBlockIndex = next;
    return true;
}

// Clears received events and block storage, but retains the send queue and last
// confirmed index. Parked-block map nodes remain with null values, so reset does
// not produce the same cursor/count state as construction.
void GameEventManager::reset()
{
    m_recvEvents.clear();

    m_nextSendBlockIndex = 0;                            // +0x07C
    m_nextRecvBlockIndex = 0;                            // +0x080
    m_sendSequence       = 0;                            // +0x090
    m_recvEventCount     = 0;                            // +0x098
    // +0x094 and +0x09C are zeroed here too; both are [UNRESOLVED] and not modelled.
    // +0x084 (m_lastConfirmedBlockIndex) is NOT - see above.

    for (std::map<int, std::list<SmartPtr<GameEvent> >*>::iterator it = m_pendingRecvBlocks.begin();
         it != m_pendingRecvBlocks.end(); ++it)
    {
        if (it->second)
        {
            it->second->clear();
            delete it->second;
        }
        it->second = 0;                                  // nulled in place, node kept
    }

    for (int i = 0; i < 32; ++i)
    {
        if (m_sentBlocks[i])
        {
            m_sentBlocks[i]->clear();
            delete m_sentBlocks[i];
        }
        m_sentBlocks[i] = 0;
    }
}

// Decode event blocks and deliver them in order, parking later blocks until gaps
// arrive. Duplicate indices fail with error 2. A stream failure clears its error
// latch before returning false. The eight-bit count permits up to 255 allocations.
// [UNRESOLVED] One header bit is read and discarded.
bool GameEventManager::processReceivedPacket(io::BitStream* stream)
{
    const unsigned int bitsBefore = stream->getBitPosition();

    unsigned int present = 0;
    stream->readBits(&present, 1);
    if (present != 1)
        return true;                                     // nothing here, and that is not an error

    unsigned int count = 0;
    stream->readBits(&count, 8);                         // up to 255

    unsigned int blockIndex = 0;
    stream->readBits(&blockIndex, 5);                    // the 32-slot window index

    unsigned int unused = 0;
    stream->readBits(&unused, 1);                        // [UNRESOLVED] - read and discarded

    if (stream->hasError())                              // reachedEndOfStream
    {
        stream->clearError();                            // hand the next manager a clean stream
        m_error = 2;
        return false;
    }

    if (count > 0)
    {
        std::list<SmartPtr<GameEvent> >* block = new std::list<SmartPtr<GameEvent> >();

        for (unsigned int i = 0; i < count; ++i)
        {
            GameEvent* const event = readGameEvent(stream);          // [FRONTIER]
            if (!event)
            {
                m_error = 2;
                delete block;
                return false;
            }
            block->insert(block->end(), SmartPtr<GameEvent>(event));     // push_back - preserve wire order (ELF 0x808F730)
            ++m_recvEventCount;
            event->release();
        }

        // A duplicate block index IS fatal - see above.
        if (m_pendingRecvBlocks.find(static_cast<int>(blockIndex)) != m_pendingRecvBlocks.end())
        {
            m_error = 2;
            delete block;
            return false;
        }
        m_pendingRecvBlocks[static_cast<int>(blockIndex)] = block;

        // drain in order, and only in order.
        for (;;)
        {
            std::map<int, std::list<SmartPtr<GameEvent> >*>::iterator it =
                m_pendingRecvBlocks.find(m_nextRecvBlockIndex & 31);
            if (it == m_pendingRecvBlocks.end())
                break;

            std::list<SmartPtr<GameEvent> >* const ready = it->second;
            insertEvents(ready);                                     // [FRONTIER]
            m_pendingRecvBlocks.erase(it);
            delete ready;
            ++m_nextRecvBlockIndex;
        }
    }

    m_readBits = stream->getBitPosition() - bitsBefore;    // +0x004
    return true;
}

} // namespace hfe
} // namespace dice
