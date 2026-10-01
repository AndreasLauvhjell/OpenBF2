// Game-event registry, shared event behavior, and local/remote event dispatch.

#include "GameEvent.h"

#include "../../IO/BitStream.h"              // readBits / writeBits

#include <cstring>                  // memcpy

#include "../../Scene/Object/PlayerManager.h"
#include "../Objects/IPlayer.h"                 // portable getId virtual dispatch
#include "../../Core/Debug.h"                      // registerEventMaker's duplicate abort
#include "../../Core/Ids.h"                        // IID_IBase - queryInterface
#include "../../Core/EventManager.h"               // eventManager->postEvent(cat, node, delay)

namespace dice {
namespace hfe {

namespace world { extern PlayerManager* playerManager; }    // Core/Globals.cpp; ds:0x8B1B928

const int GEPostRemote = 11;        // .rodata 0x8889FE4 (_ZN4dice3hfe12GEPostRemoteE) - getType's value

// GameEvent ctor - ELF 0x8086C10, 28 bytes: refcount 1, field08 -1.
GameEvent::GameEvent()
{
    m_refCount = 1;
    m_sendSequence = -1;      // "never queued" - GameEventManager stamps it
}

// Default execution has no client effect and succeeds on the server.
void GameEvent::executeClient()       {}
bool GameEvent::executeServer(int)    { return true; }

void  GameEvent::addRef()                       { ++m_refCount; }
int   GameEvent::getRef()                       { return m_refCount; }
int   GameEvent::release()
{
    const int left = m_refCount - 1;
    const bool last = (m_refCount == 1);
    m_refCount = left;
    if (last)
        delete this;                                   // the virtual dtor (D0)
    return left;
}
void* GameEvent::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase)
        return const_cast<GameEvent*>(this);
    return 0;
}
void  GameEvent::logInfo(bool, int)             {}
void  GameEvent::eventReceivedByRemote(void*)   {}

// Marks the event local only when playerId matches the local human, causing
// immediate execution during construction. Requires playerManager.
PostRemoteEvent::PostRemoteEvent(EventCategory category, unsigned int eventId, int playerId,
                                 void* payload, unsigned int payloadBitSize, float f)
{
    m_eventCategory  = (int)category;              // +0x0C
    m_field4C        = f;                          // +0x4C
    m_eventId        = eventId;                    // +0x10
    m_payloadBitSize = (unsigned char)payloadBitSize;   // +0x48 - stored as a byte, as shipped

    world::IPlayer* player = world::playerManager->getLocalHumanPlayer();   // vtable +0x30
    bool isLocal = false;
    if (player && player->getId() == playerId)  // ELF +0x54 / PE +0x50
        isLocal = true;
    ConstructorHelper(category, eventId, isLocal, payload, payloadBitSize);
}

// Builds the event with caller-supplied local-player status, avoiding a player lookup.
PostRemoteEvent::PostRemoteEvent(EventCategory category, unsigned int eventId, bool isLocal,
                                 void* payload, unsigned int payloadBitSize, float f)
{
    m_eventCategory  = (int)category;              // +0x0C
    m_field4C        = f;                          // +0x4C
    m_eventId        = eventId;                    // +0x10
    m_payloadBitSize = (unsigned char)payloadBitSize;   // +0x48
    ConstructorHelper(category, eventId, isLocal, payload, payloadBitSize);
}

// Copies payload and immediately executes local events.
// Despite its name, payloadBitSize is used as a byte count; current callers pass zero.
void PostRemoteEvent::ConstructorHelper(EventCategory, unsigned int, bool isLocal,
                                        void* payload, unsigned int payloadBitSize)
{
    if (payload)
        memcpy(m_payload, payload, payloadBitSize);
    if (isLocal)
        executeClient();                           // virtual dispatch, slot +0x24
}

// Delayed remote events allocate a node; immediate dispatch reuses one static node
// with borrowed payload storage. Only ECNetwork event 20 posts a delivery acknowledgment.
namespace {
StaticRemoteEvent* g_staticRemoteEvent = new StaticRemoteEvent();   // ds:0x8AAB11C [CHOSEN name]

inline EventNode* remoteEventNodeFor(PostRemoteEvent* event, unsigned int eventId, void* payload,
                                     float delay, int connectionId)
{
    RemoteEvent* node;
    if (delay > 0.0f)
        node = new RemoteEvent();                  // operator new(0x28); m_next = 0 (+0x18)
    else
        node = g_staticRemoteEvent;                // the shared node, mutated in place
    node->m_id           = eventId;                // +0x1C
    node->m_data         = payload;                // +0x20 - the event's OWN buffer, by address
    node->m_connectionId = connectionId;           // +0x24
    (void)event;
    return node;
}
} // namespace

// The maker's form - see the header. The ELF has no such constructor.
PostRemoteEvent::PostRemoteEvent()
    : m_eventCategory(0)
    , m_eventId(0)
    , m_payloadBitSize(0)
    , m_field4C(0.0f)
{
    std::memset(m_payload, 0, sizeof(m_payload));
}

int  PostRemoteEvent::getType() const { return GEPostRemote; }
void PostRemoteEvent::logInfo(bool, int) {}

void PostRemoteEvent::executeClient()
{
    EventNode* node = remoteEventNodeFor(this, m_eventId, m_payload, m_field4C, -1);
    eventManager->postEvent(static_cast<EventCategory>(m_eventCategory), node, m_field4C);   // +0x10
}

bool PostRemoteEvent::executeServer(int connectionId)
{
    EventNode* node = remoteEventNodeFor(this, m_eventId, m_payload, m_field4C, connectionId);
    eventManager->postEvent(static_cast<EventCategory>(m_eventCategory), node, m_field4C);   // +0x10
    return true;
}

void PostRemoteEvent::eventReceivedByRemote(void* arg)
{
    const int connectionId = *static_cast<int*>(arg);
    if (m_eventCategory == 6 && m_eventId == 20)                                       // ECNetwork, event 20
    {
        g_staticRemoteEvent->m_id           = 20;                                      // +0x1C
        g_staticRemoteEvent->m_connectionId = connectionId;                            // +0x24
        g_staticRemoteEvent->m_data         = m_payload;                               // +0x20
        eventManager->postEvent(static_cast<EventCategory>(m_eventCategory), g_staticRemoteEvent, 0.0f);
    }
}


namespace {
// Fallback event name is Unknown event. Preserve namespace-scope dynamic initialization.
const std::string g_unknownEventName("Unknown event");
} // namespace

// Create the registry before event serialization can use it. [INFERRED] global name.
GameEventRegistry* gameEventRegistry = new GameEventRegistry();

// gameEventTypeIdBitCount - inlined AT every site IN THE ELF; factored into one function here.
// See the long note on the declaration in GameEvent.h for why the `if` guard is not the same as the
// bare do/while loops ConnectionTypeEvent and CommanderEvent carry, and for why the answer is 7
// once Game::registerGameEvents has run.
unsigned int gameEventTypeIdBitCount()
{
    unsigned int bits = 0;
    const unsigned int highest = gameEventRegistry->getHighestEventId();   // no null check - as shipped
    if (highest)
    {
        do
            ++bits;
        while (highest > (1u << bits) - 1u);
    }
    return bits;
}

//  (C2) / 0x8086580 (C1), 333 / 165 B. The two differ only in how much of the node
// allocator GCC inlined; both do the same four things.
// THE refcount IS born AT 1, like every other IBase in this engine.
GameEventRegistry::GameEventRegistry()
    : m_refCount(1)                                    // +0x04
    , m_highestEventId(0)                              // +0x20
{
    // The two maps' header nodes are allocated by their own constructors here; the ELF open-codes
    // that (`allocate(24)`, then colour/parent/left/right on the new node) because libstdc++'s
    // _Rb_tree constructor is inline. Compiler scaffolding, not behaviour.
}

// Destroys name and maker maps through member destruction. Makers themselves are
// borrowed file-scope instances and are not deleted.
GameEventRegistry::~GameEventRegistry()
{
}

void GameEventRegistry::addRef()
{
    ++m_refCount;
}

int GameEventRegistry::getRef()
{
    return m_refCount;
}

int GameEventRegistry::release()
{
    const int refs = --m_refCount;
    if (refs == 0)
        delete this;
    return refs;
}

// . IID_IBase and nothing else - the registry publishes no interface of its own.
void* GameEventRegistry::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase)
        return const_cast<GameEventRegistry*>(this);
    return 0;
}

// Registers an event maker and name, updating the highest event id. Duplicate ids
// raise a fatal diagnostic. Returns true on every continuing path.
bool GameEventRegistry::registerEventMaker(unsigned int id, const std::string& name,
                                           IGameEventMaker* maker)
{
    if (m_eventMakers.find(id) != m_eventMakers.end())
    {
        Debug(DEBUGTYPE_6, "Game/Common/GameEvent.cpp", 53, "")
            << "attempting to register an already registred type" << id;   // sic - see above
    }

    m_eventMakers.insert(std::make_pair(id, maker));    // insert_unique: a duplicate is dropped
    m_eventNames.insert(std::make_pair(id, name));

    if (id > m_highestEventId)
        m_highestEventId = id;

    return true;
}

// getEventMaker - 0x8086B80, 62 B. A miss returns null, and GameEventManager::readGameEvent treats
// that as "drop the packet" rather than as an error.
IGameEventMaker* GameEventRegistry::getEventMaker(unsigned int id) const
{
    std::map<unsigned int, IGameEventMaker*>::const_iterator it = m_eventMakers.find(id);
    if (it == m_eventMakers.end())
        return 0;
    return it->second;
}

// getEventName - 0x8086BC0, 71 B. A miss returns a reference to the shared "Unknown event" string
// at ds:0x8AAB5B8 - NOT an empty string, and NOT a temporary. The reference outlives the call
// because the object is a namespace-scope static; the same shape dice::hfe::getEmptyString and
// Vec3::getZero use.
const std::string& GameEventRegistry::getEventName(unsigned int id) const
{
    std::map<unsigned int, std::string>::const_iterator it = m_eventNames.find(id);
    if (it == m_eventNames.end())
        return g_unknownEventName;
    return it->second;
}


// Serializes category, id, float bits, then a byte count and payload.
// [CHOSEN, not recovered] Bounds incoming payloads to local storage and skips excess
// bytes so subsequent events remain aligned. The wire count can exceed this buffer.
bool PostRemoteEvent::deSerialize(io::BitStream* stream)
{
    int category = 0;
    stream->readBits(&category, 4);
    m_eventCategory = category;

    unsigned int eventId = 0;
    stream->readBits(&eventId, 32);
    m_eventId = eventId;

    stream->readBits(&m_field4C, 32);            // straight into the float

    int count = 0;
    stream->readBits(&count, 8);

    const unsigned int onTheWire = static_cast<unsigned int>(count) & 0xFFu;
    unsigned int stored = onTheWire;
    if (stored > sizeof(m_payload))
        stored = sizeof(m_payload);              // the divergence - see above

    m_payloadBitSize = static_cast<unsigned char>(stored);
    if (onTheWire)                               // the ELF skips the read entirely at zero
    {
        stream->readBits(m_payload, stored * 8);
        if (onTheWire > stored)
            stream->skipBits((onTheWire - stored) * 8);   // keep the cursor where the ELF puts it
    }

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

bool PostRemoteEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int category = m_eventCategory;
    stream->writeBits(&category, 4);

    unsigned int eventId = m_eventId;
    stream->writeBits(&eventId, 32);

    stream->writeBits(&m_field4C, 32);

    int count = m_payloadBitSize;
    stream->writeBits(&count, 8);

    if (m_payloadBitSize)
        stream->writeBits(m_payload, static_cast<unsigned int>(m_payloadBitSize) * 8);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
