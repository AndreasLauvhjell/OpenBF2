// Local engine-event listener with handleEvent as its sole virtual slot.
// The protected nonvirtual destructor prevents deletion through this interface.
// These event categories are separate from network GameEvent wire IDs.
// [UNRESOLVED] handleEvent's void return remains provisional.

#ifndef GAME_IEVENTLISTENER_H       // [INFERRED] guard style
#define GAME_IEVENTLISTENER_H

namespace dice {
namespace hfe {

// Event category ids index EventManager's fixed twelve-category tables. Preserve
// registered category/event names, including the shared ECAIi names.
// [UNRESOLVED] Unnamed categories remain numeric until their names are confirmed.
enum EventCategory : int        // `: int` is MSVC's default; spelled so the opaque declarations that name
{                               //   this enum (IGameLogic.h, BF2Engine.h) agree with it under GCC too
    ECGameLogic = 0,
    ECGame      = 1,
    ECHud       = 2,    // [CHOSEN, not recovered] - no ECHud string in the server ELF; see above
    ECAI        = 3,
    // Client sound events; the dedicated sound facade registers no handlers.
    ECSound     = 4,
    ECCore      = 5,
    ECNetwork   = 6,
    // Editor events, including heightmap modification notifications.
    ECEditor    = 8,
    ECPhysics   = 9
};

class EventNode;    // [UNRESOLVED] layout

// Keep handleEvent first in the virtual declaration order.
class IEventListener
{
public:
    virtual void handleEvent(EventCategory category,
                             unsigned int  id,
                             EventNode*    node,
                             void*         userData) = 0;

    // Protected nonvirtual destructor; deletion through this interface is prohibited.
protected:
    ~IEventListener() {}
};

} // namespace hfe
} // namespace dice

#endif
