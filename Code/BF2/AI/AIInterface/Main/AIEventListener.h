// Routes engine events to AI couriers. BFConsole owns this listener and its event registration.

#ifndef AI_AIINTERFACE_MAIN_AIEVENTLISTENER_H
#define AI_AIINTERFACE_MAIN_AIEVENTLISTENER_H

#include "../../../Game/IEventListener.h"

namespace dice {
namespace hfe {
namespace ai {

// sizeof == 8, from `operator new(8u)` in BFConsole::createSingletons: a vptr and two bytes.
class AIEventListener : public IEventListener
{
public:
    AIEventListener();
    virtual ~AIEventListener();

    // Receives engine events through IEventListener's first virtual slot; preserve slot order.
    virtual void handleEvent(EventCategory category,
                             unsigned int  id,
                             EventNode*    node,
                             void*         userData);

    void init();                        // - not virtual
    static void registerEvents();       // Registers through the global event manager; no instance state.

private:
    /* +0x00 */ // vptr
    // [UNRESOLVED] Set by construction and init; no reader or clear operation is identified.
    /* +0x04 */ bool m_initialised;
    // [INFERRED] After category 5/id 6, category 1/id 2 reports disconnects only for AI
    // players. Initially all disconnects are reported; this latch is never cleared.
    /* +0x05 */ bool m_unknown05;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIINTERFACE_MAIN_AIEVENTLISTENER_H
