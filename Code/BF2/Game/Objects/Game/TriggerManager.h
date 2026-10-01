// [INFERRED] beside the attested Game/Objects/Game/Trigger.cpp.
// world::TriggerManager, not anim::TriggerManager. ELF ZTV88A2C70 has
// only IEventListener::handleEvent; its destructor is not virtual.
#ifndef GAME_OBJECTS_GAME_TRIGGERMANAGER_H
#define GAME_OBJECTS_GAME_TRIGGERMANAGER_H

#include "Trigger.h"
#include "../../IEventListener.h"
#include <list>
#include <set>

namespace dice { namespace hfe { namespace world {
class TriggerManager : public IEventListener
{
public:
    TriggerManager();
    ~TriggerManager();
    void removeAll();
    void setupTriggerLinks();
    void addTrigger(Trigger* trigger);
    void removeTrigger(Trigger* trigger);
    void handleEvent(TriggerType type, IObject* source, int triggerId);
    void handleUpdate(float deltaTime, unsigned int tick);
    virtual void handleEvent(EventCategory, unsigned int, EventNode*, void*);

private:
    std::list<Trigger*> m_triggers;              // ELF +04 / VC7.1 +04
    std::set<Trigger*> m_activeTriggers;         // ELF +08 / VC7.1 +10
    std::list<Trigger*>::iterator m_nextTrigger; // ELF +14 / VC7.1 +1C
};
extern TriggerManager* triggerManager;
} } }
#endif
