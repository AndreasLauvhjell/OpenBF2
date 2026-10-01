// Schedules and dispatches events; posting returns a handle accepted by removeEvent.
// Header path [INFERRED]. The x86 ELF object is 260 bytes. Preserve virtual declaration order:
// the destructor follows the functional methods, and ELF/MSVC reverse the same-name overload pairs.
// Use typed calls instead of shared raw slot offsets.
// EventManager and FrameEventManager are separate registered instances; the latter handles category 10.
// [UNRESOLVED] the reason for that split beyond separate routing.
// [UNRESOLVED] the IEventManager base declaration; no additional base subobject is modeled.

#ifndef CORE_EVENTMANAGER_H         // [INFERRED] guard style
#define CORE_EVENTMANAGER_H

#include <map>
#include <string>

#include "../Game/IEventListener.h"     // EventCategory, EventNode, IEventListener

namespace dice {
namespace hfe {

class IBase;

// Queued event node. Field names and header placement are [INFERRED].
// The x86 ELF node is 28 bytes; MSVC retains member offsets but pads its size to 32 bytes.
class EventNode
{
public:
    EventNode() : m_next(0) {}                  // the ELF ctor stores only the vptr and m_next = 0

    virtual ~EventNode() {}                     // +0x00 / +0x04
    virtual void release() { if (this) delete this; }   // +0x08   0x8077EB0 - `if (this)` is in the
                                                //                  binary, redundant and preserved
    virtual int  getType() const = 0;           // +0x0C   pure (__cxa_pure_virtual in the vtable)

    EventCategory m_category;     // +0x04  set by postEvent, re-read by dispatchEvent every pass
    double        m_time;         // +0x08  due time while queued; overwritten with the manager's
                                  //        current time by dispatchEvent just before delivery
    unsigned int  m_updateStamp;  // +0x10  EventManager::m_updateCount at the moment of posting
    unsigned int  m_eventId;      // +0x14  the handle insertEvent hands back / removeEvent takes
    EventNode*    m_next;         // +0x18  singly-linked, sorted ascending by m_time
};

// 36 bytes in the ELF (0x24) - `operator new(0x24u)` in both postEvent overloads. getType() returns
// -1, which is dispatchEvent's sentinel for "the id and userData are stored in the node itself".
class NotificationEventNode : public EventNode
{
public:
    NotificationEventNode() {}                                  // m_id/m_data deliberately unset -
                                                                // the embedded member is built this
                                                                // way and postEvent fills it later
    NotificationEventNode(unsigned int id, void* data) : m_id(id), m_data(data) {}

    virtual int getType() const { return -1; }

    unsigned int m_id;     // +0x1C (ELF)  the event id handed to IEventListener::handleEvent
    void*        m_data;   // +0x20 (ELF)  the userData handed to IEventListener::handleEvent
};

// EventNode carrying the event ID dispatched to listeners; base of HeightmapEventNode.
// Header placement is [INFERRED] and the member name is [CHOSEN].
class SimpleEventNode : public EventNode
{
public:
    explicit SimpleEventNode(unsigned int type) : m_type(type) {}

    virtual int getType() const { return static_cast<int>(m_type); }

    unsigned int m_type;   // +0x1C (ELF)
};

class EventManager
{
public:
    struct Handler;                                     // defined at the bottom of the class

    EventManager();

    // Vtable, in slot order. Do not reorder.
    virtual void  addRef();                                                     // +0x00
    virtual int   getRef();                                                     // +0x04
    virtual int   release();                                                    // +0x08
    virtual void* queryInterface(unsigned int) const;                           // +0x0C

    // return type (was void on all four): each returns the pending-event
    // handle - insertEvent's value, tail-returned - or 0 when the event was dispatched immediately,
    // dropped for want of a handler, or the node was null. See correction (1) at the top.
    virtual unsigned int postEvent(EventCategory, EventNode*, float);           // ELF +0x10, PE +0x14
    virtual unsigned int postEvent(EventCategory, unsigned int, void*, float);  // ELF +0x14, PE +0x10
    virtual unsigned int postEventNextUpdate(EventCategory, EventNode*);        // ELF +0x18, PE +0x1C
    virtual unsigned int postEventNextUpdate(EventCategory, unsigned int, void*);// ELF +0x1C, PE +0x18

    // return type `mov eax, 1` before the epilogue on every path.
    // BF2Engine::mainLoop discards it (0x813757E), which is why it read as void.
    virtual bool  processEvents(float);                                         // +0x20
    virtual void  removePendingEvents();                                        // +0x24
    virtual bool  removeEvent(unsigned int);                                    // +0x28

    // Registers a listener in ascending priority order. Returns whether registration succeeds.
    virtual bool  registerEventHandler(EventCategory, IEventListener*, int priority);  // +0x2C
    virtual bool  unregisterEventHandler(EventCategory, IEventListener*);              // +0x30

    // return type both return 1/0 explicitly (false = duplicate id, and
    // false = "not registered" respectively, each with a Debug of its own).
    virtual bool  registerEvent(EventCategory, unsigned int, const std::string&);      // +0x34
    virtual bool  unregisterEvent(EventCategory, unsigned int);                        // +0x38

    virtual ~EventManager();                                                    // +0x3C / +0x40

    // non-virtual
    void dispatchEvent(EventNode*);
    void popEvent();
    unsigned int insertEvent(EventNode*, float);        // returns the new handle
    void unregisterAllHandlers();                       // no caller in the binary

    // Module factory for EventManager.
    static IBase* create(unsigned int, IBase*);

private:
    // The twelve-category layout occupies 0x104 bytes on x86. Handler guards accept category 12,
    // which indexes beyond m_handlers into m_notificationNode; the existing defect is preserved.
    int                   m_refCount;         // +0x04  1 at construction
    unsigned int          m_updateCount;      // +0x08  ++ at the top of processEvents; stamped into
                                              //        every posted node so that events posted
                                              //        during an update wait for the next one
    int                   m_dispatchDepth;    // +0x0C  ++/-- around dispatchEvent; non-zero means
                                              //        m_notificationNode is in use (reentrancy)
    EventNode*            m_pendingEvents;    // +0x10  head of the time-sorted queue
    double                m_time;             // +0x14  accumulated frame time; the queue's clock
    Handler*              m_handlers[12];     // +0x1C  per-category listener lists, priority-sorted
    NotificationEventNode m_notificationNode; // +0x4C  the reusable node for the no-delay,
                                              //        non-reentrant postEvent(cat,id,data,0) path
    unsigned int          m_nextEventId;      // +0x70  handle counter, starts at 1
    std::map<unsigned int, std::string> m_eventNames[12];  // +0x74  12 x 12-byte GCC 3.x _Rb_tree.
                                              //        0x74 + 144 = 0x104 exactly.

public:
    // Owned listener-list node. [CHOSEN, not recovered] Type name; layout follows the x86 reference.
    struct Handler
    {
        IEventListener* m_listener;   // +0x00
        Handler*        m_next;       // +0x04
        int             m_priority;   // +0x08  ascending; ties keep insertion order
    };
};

extern EventManager* eventManager;          // ds:0x8AC85E0
extern EventManager* frameEventManager;     // ds:0x8AC85E4

} // namespace hfe
} // namespace dice

#endif // CORE_EVENTMANAGER_H
