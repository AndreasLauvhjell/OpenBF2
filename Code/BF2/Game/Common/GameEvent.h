// Reference-counted game events and local/remote payload delivery. Serialization
// uses the registry's event-id width. PostRemoteEvent requires PlayerManager and
// may execute synchronously during construction when addressed to the local player.
// Its inline payload capacity is 52 bytes.
// [UNRESOLVED tree-wide] executeClient's void declaration differs from observed
// return materialization; changing it requires every override and caller.

#ifndef GAME_COMMON_GAMEEVENT_H     // [INFERRED] guard style
#define GAME_COMMON_GAMEEVENT_H

#include <map>                      // GameEventRegistry's two tables
#include <string>

#include "../IEventListener.h"      // EventCategory - the recovered enum (waitForClients' category
                                    // 6 == ECNetwork)
#include "../../Core/EventManager.h" // NotificationEventNode - RemoteEvent's base
#include "../../Core/IBase.h"        // GameEventRegistry's base

namespace dice {
namespace hfe {

namespace io { class BitStream; }

// Returns the bits required for the registry's highest event ID. Requires a live registry.
// An empty registry yields zero bits; event registration must precede any wire encoding.
unsigned int gameEventTypeIdBitCount();

class GameEvent
{
public:
    GameEvent();

    // The quartet - (0x8069570 / 0x8069580 / 0x8069590 / 0x80695D0): a plain
    // refcount at +0x04; release deletes through the virtual dtor at zero; queryInterface answers
    // IID_IBase only.
    virtual void  addRef();                            // +0x00  15 B
    virtual int   getRef();                            // +0x04  11 B
    virtual int   release();                           // +0x08  57 B
    virtual void* queryInterface(unsigned int) const;  // +0x0C  20 B
    virtual ~GameEvent() {}                            // +0x10/+0x14  [CHOSEN] trivial - the class
                                                       // owns nothing needing destruction
    virtual int  getType() const = 0;                  // +0x18  pure in the base (dumped)
    virtual void logInfo(bool, int);                   // +0x1C   - empty
    virtual void eventReceivedByRemote(void*);         // +0x20   - empty
    virtual void executeClient();                      // +0x24  : return 1 (void here -
                                                       //        see the vtable block)
    virtual bool executeServer(int connectionId);      // +0x28  : return 1
    virtual bool deSerialize(io::BitStream* stream) = 0;   // +0x2C  pure
    virtual bool serialize(io::BitStream* stream) = 0;     // +0x30  pure

    int m_refCount;                                    // +0x04  ctor -> 1
    // Per-manager sequence assigned when queued or delivered. -1 means never queued;
    // receiver block ordering uses a separate block index.
    int m_sendSequence;                                // +0x08  ctor -> -1
};

class PostRemoteEvent : public GameEvent
{
public:
    // Default form for event makers; initializes members that deserialization replaces.
    PostRemoteEvent();

    PostRemoteEvent(EventCategory category, unsigned int eventId, int playerId,
                    void* payload, unsigned int payloadBitSize, float f);
    PostRemoteEvent(EventCategory category, unsigned int eventId, bool isLocal,
                    void* payload, unsigned int payloadBitSize, float f);
    virtual int  getType() const;                      // GEPostRemote (11)
    virtual void logInfo(bool, int);                   // Intentionally empty.
    virtual void eventReceivedByRemote(void*);
    virtual void executeClient();
    virtual bool executeServer(int connectionId);      // Returns true.
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

    int           m_eventCategory;                     // +0x0C
    unsigned int  m_eventId;                           // +0x10
    unsigned char m_payload[0x34];                     // +0x14..+0x47 - ConstructorHelper's memcpy
                                                       //        target
    unsigned char m_payloadBitSize;                    // +0x48 - stored as a byte (mov [ebx+48h],al)
    float         m_field4C;                           // +0x4C [UNRESOLVED meaning - a float, 0.0f
                                                       //        from waitForClients]

private:
    void ConstructorHelper(EventCategory, unsigned int, bool isLocal,
                           void* payload, unsigned int payloadBitSize);
};

// Notification nodes used to execute PostRemoteEvent payloads. Delayed events own heap nodes;
// immediate events reuse one static node whose release is inert. Payload pointers are borrowed,
// and another immediate dispatch can overwrite the shared node's ID and data.
class RemoteEvent : public NotificationEventNode
{
public:
    RemoteEvent() : m_connectionId(-1) {}
    virtual ~RemoteEvent() {}                          //  / 0x8077F40 (the vptr re-seat + delete)
    int m_connectionId;                                // +0x24
};

class StaticRemoteEvent : public RemoteEvent
{
public:
    virtual ~StaticRemoteEvent() {}
    virtual void release() {}                          // the static node is never freed
};

// One-method maker interface without a virtual destructor. Makers have static lifetime.
class IGameEventMaker
{
public:
    virtual GameEvent* createEvent() = 0;              // vtable +0x00 - the only slot
protected:
    ~IGameEventMaker() {}                              // non-virtual: see above
};

// Default-initializes an event for deserialization. Fields without constructor initialization
// remain unspecified until decoded; conditional payload forms must not read each other's fields.
// Some event constructors explicitly zero fields, so their initial state differs from retail.
template <class T>
class GameEventMakerMaker : public IGameEventMaker
{
public:
    virtual GameEvent* createEvent() { return new T; } // NO parentheses - see above
};

// Maps network event IDs to borrowed static makers and registered names.
// Registration updates both maps; duplicate IDs are fatal.
// STL-dependent object sizes and offsets are specific to the active compiler.
class GameEventRegistry : public IBase
{
public:
    GameEventRegistry();
    virtual ~GameEventRegistry();                      //  / 0x8086700, D0 at 0x80867D0

    // the IBase quartet, +0x00..+0x0C
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();                           // deletes at zero
    virtual void* queryInterface(unsigned int) const;  // IID_IBase only

    // non-virtual
    // none of these three is in the vtable. The six slots above are the whole table; every caller
    // reaches these by direct call on the concrete type, which is why registerGameEvents can hold
    // the global as a GameEventRegistry* rather than an IBase*.
    bool registerEventMaker(unsigned int id, const std::string& name, IGameEventMaker* maker);
    IGameEventMaker*   getEventMaker(unsigned int id) const;
    const std::string& getEventName(unsigned int id) const;

    unsigned int getHighestEventId() const { return m_highestEventId; }
                                                       // [CHOSEN] - the ELF reads the field inline
                                                       // at every site; there is no accessor symbol

private:
    /* +0x04 */ int                                       m_refCount;
    /* +0x08 */ std::map<unsigned int, IGameEventMaker*>  m_eventMakers;
    /* +0x14 */ std::map<unsigned int, std::string>       m_eventNames;
    /* +0x20 */ unsigned int                              m_highestEventId;
};

// [INFERRED] Shared event registry, populated by Game::registerGameEvents before packet handling.
extern GameEventRegistry* gameEventRegistry;

} // namespace hfe
} // namespace dice

#endif
