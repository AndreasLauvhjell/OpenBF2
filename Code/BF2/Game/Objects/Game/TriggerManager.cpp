// World trigger-manager lifecycle and event handling.
#include "TriggerManager.h"
#include "../ObjectLayoutAccess.h"
#include "../../../Core/EventManager.h"

namespace dice { namespace hfe { namespace world {

// ELF 81C9AF0 / PE 543480. Containers are non-owning. Publication is first-wins.
TriggerManager::TriggerManager()
{
    if (!triggerManager)
        triggerManager = this;
    m_nextTrigger = m_triggers.begin();
}

// ELF 81C9C40 / PE 542D00. Unregistration is unconditional, even for a
// second instance that did not publish itself. Retail has no matching ctor registration.
TriggerManager::~TriggerManager()
{
    if (triggerManager == this)
        triggerManager = 0;
    eventManager->unregisterEventHandler(ECCore, this);
}

// ELF 81C9D00. Do not destroy or release the indexed objects.
void TriggerManager::removeAll()
{
    m_triggers.clear();
    m_activeTriggers.clear();
    m_nextTrigger = m_triggers.begin();
}

// ELF 81C9D70 / PE 542D70. Collect iterators before erasing invalid links.
void TriggerManager::setupTriggerLinks()
{
    std::list<std::list<Trigger*>::iterator> invalid;
    for (auto it = m_triggers.begin(); it != m_triggers.end(); ++it)
    {
        Trigger* trigger = *it;
        trigger->findEffectors();
        if (!trigger->isValid())
            invalid.push_back(it);
    }
    for (auto it = invalid.begin(); it != invalid.end(); ++it)
        m_triggers.erase(*it);
    m_nextTrigger = m_triggers.begin();
}

// ELF 81C9EA0 / PE 542EA0. Deliberately no duplicate check or cursor reset.
void TriggerManager::addTrigger(Trigger* trigger)
{
    m_triggers.push_back(trigger);
}

// ELF 81C9EE0 / PE 542EE0. The active-set scan is inside each matching
// list entry. Preserve that ordering, including retail's duplicate-active-iterator defect.
void TriggerManager::removeTrigger(Trigger* trigger)
{
    std::list<std::list<Trigger*>::iterator> removed;
    std::list<std::set<Trigger*>::iterator> inactive;
    for (auto it = m_triggers.begin(); it != m_triggers.end(); ++it)
    {
        if (*it == trigger)
        {
            for (auto active = m_activeTriggers.begin(); active != m_activeTriggers.end(); ++active)
                if (*active == trigger)
                    inactive.push_back(active);
            removed.push_back(it);
        }
    }
    for (auto it = removed.begin(); it != removed.end(); ++it)
        m_triggers.erase(*it);
    for (auto it = inactive.begin(); it != inactive.end(); ++it)
        m_activeTriggers.erase(*it);
    m_nextTrigger = m_triggers.begin();
}

// ELF 81CA110 / PE 542BB0. Noisy weapons visit the active set; object and
// control-point events visit the full list. Control points have no distance test.
void TriggerManager::handleEvent(TriggerType type, IObject* source, int triggerId)
{
    if (!source)
        return;
    if (type == TriggerType_Weapon)
    {
        for (auto it = m_activeTriggers.begin(); it != m_activeTriggers.end(); ++it)
        {
            Trigger* trigger = *it;
            const TriggerTemplate* objectTemplate = static_cast<const TriggerTemplate*>(
                object_detail::LayoutAccess::objectTemplate(trigger));
            if ((triggerId == -1 || objectTemplate->m_triggerId == triggerId)
                && (trigger->getTypes() & type)
                && trigger->isInArea(source->getAbsolutePosition()))
                trigger->updateOnEvent(type, source);
        }
    }
    else if (type == TriggerType_ControlPoint || type == TriggerType_Object)
    {
        for (auto it = m_triggers.begin(); it != m_triggers.end(); ++it)
        {
            Trigger* trigger = *it;
            const TriggerTemplate* objectTemplate = static_cast<const TriggerTemplate*>(
                object_detail::LayoutAccess::objectTemplate(trigger));
            if ((triggerId == -1 || objectTemplate->m_triggerId == triggerId)
                && (trigger->getTypes() & type))
            {
                if (type == TriggerType_ControlPoint || trigger->isInArea(source->getAbsolutePosition()))
                    trigger->updateOnEvent(type, source);
            }
        }
    }
}

// ELF 81CA300 / PE 543160. At most three list entries per call, without
// wrapping mid-pass. Newly active triggers are inserted after the old active set's update.
void TriggerManager::handleUpdate(float, unsigned int)
{
    if (m_nextTrigger == m_triggers.end())
        m_nextTrigger = m_triggers.begin();
    std::list<Trigger*> activated;
    std::list<Trigger*> unused; // Present in both shipped bodies.
    int tested = 0;
    while (m_nextTrigger != m_triggers.end())
    {
        Trigger* trigger = *m_nextTrigger;
        if (m_activeTriggers.find(trigger) == m_activeTriggers.end() && trigger->checkTriggerArea())
        {
            activated.push_back(trigger);
            if ((trigger->getTypes() & 1) || (trigger->getTypes() & 4))
                trigger->updateOnTime();
        }
        ++tested;
        ++m_nextTrigger;
        if (tested >= 3)
            break;
    }
    std::list<Trigger*> deactivated;
    for (auto it = m_activeTriggers.begin(); it != m_activeTriggers.end(); ++it)
    {
        Trigger* trigger = *it;
        if (trigger->checkTriggerArea())
        {
            if ((trigger->getTypes() & 1) || (trigger->getTypes() & 4))
                trigger->updateOnTime();
        }
        else
            deactivated.push_back(trigger);
    }
    for (auto it = activated.begin(); it != activated.end(); ++it)
        m_activeTriggers.insert(*it);
    for (auto it = deactivated.begin(); it != deactivated.end(); ++it)
        m_activeTriggers.erase(*it);
}

// ELF81CA640: push ebp; mov ebp,esp; pop ebp; ret. PE542B30: ret 10h.
// The sole virtual slot (PE table7CB578); this empty body is shipped behavior.
void TriggerManager::handleEvent(EventCategory, unsigned int, EventNode*, void*) {}

} } }
