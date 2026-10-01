// [INFERRED] placement beside Trigger.cpp. ELF81C37D0..81C4C7A,
// dedicated PE ctor546370, dtor5461F0, update545680, reset545500,
// query5462B0, add546450. Native non-owning STL registries.
#include "TriggerableManager.h"
#include "Triggerable.h"
#include "../TemplateLeaves.h"
#include "../../IGameLogic.h"
#include "../../../Core/Ids.h"
#include "../../../Core/StringUtil.h"

namespace dice { namespace hfe {
extern IGameLogic* gameLogic;
namespace world {

TriggerableManager::TriggerableManager() : m_nextTarget()
{
    if (!triggerableManager)
        triggerableManager = this;
    m_nextTriggerable = m_triggerables.begin();
    // Both retail ctors leave the target cursor singular; reset establishes begin().
}

TriggerableManager::~TriggerableManager()
{
    if (triggerableManager == this)
        triggerableManager = 0;
}

// ELF81C3E50 / PE545680. Three list entries, then the entire previously active set.
void TriggerableManager::handleUpdate(float deltaTime, unsigned int)
{
    if (m_nextTriggerable == m_triggerables.end())
        m_nextTriggerable = m_triggerables.begin();
    std::list<Triggerable*> activated;
    std::list<Triggerable*> unused; // Constructed and destroyed, unused in both ports.
    int tested = 0;
    while (m_nextTriggerable != m_triggerables.end())
    {
        Triggerable* trigger = *m_nextTriggerable;
        if (m_active.find(trigger) == m_active.end()
            && (trigger->getTypes() == 1 || trigger->getTypes() == 3))
        {
            if (!trigger->m_template->m_bool19E || !hasActivePartner(trigger))
            {
                if (trigger->checkTriggerableArea())
                {
                    trigger->m_bool16C = !trigger->m_bool16C;
                    if (gameLogic->isServer())
                        activated.push_back(trigger);
                }
            }
        }
        ++tested;
        ++m_nextTriggerable;
        if (tested >= 3)
            break;
    }
    std::list<Triggerable*> deactivated;
    for (auto it = m_active.begin(); it != m_active.end(); ++it)
    {
        Triggerable* trigger = *it;
        if (trigger->handleTrigger(deltaTime))
            deactivated.push_back(trigger);
    }
    for (auto it = activated.begin(); it != activated.end(); ++it)
    {
        if (!alreadyActive(*it))
        {
            m_active.insert(*it);
            (*it)->resetTriggerTargets();
        }
    }
    for (auto it = deactivated.begin(); it != deactivated.end(); ++it)
        m_active.erase(*it);
}

// ELF81C41D0 / PE545340. Preserve the two copies, virtual names, and c-string comparison.
bool TriggerableManager::hasActivePartner(Triggerable* trigger)
{
    const std::string name(trigger->m_template->getName());
    for (auto it = m_active.begin(); it != m_active.end(); ++it)
    {
        const std::string other((*it)->m_template->getName());
        if (compareIgnoreCase(name.c_str(), other.c_str()) == 0)
            return true;
    }
    return false;
}

// ELF81C4320. Reset is after insertion and only on the inactive edge.
void TriggerableManager::activateTrigger(Triggerable* trigger)
{
    if (!alreadyActive(trigger))
    {
        m_active.insert(trigger);
        trigger->resetTriggerTargets();
    }
}

// ELF81C4380 / PE545450: a linear scan, not set::find.
bool TriggerableManager::alreadyActive(Triggerable* trigger)
{
    for (auto it = m_active.begin(); it != m_active.end(); ++it)
        if (*it == trigger)
            return true;
    return false;
}

void TriggerableManager::addTriggerable(Triggerable* trigger)
{
    m_triggerables.push_back(trigger);
    m_byId[trigger->getTriggerId()] = trigger; // TEMPLATE id, not the runtime sequence id.
}

void TriggerableManager::addTriggerableTemplate(TriggerableTemplate* objectTemplate)
{
    m_templates.push_back(objectTemplate);
}

// ELF81C44D0 / PE545A40. Like TriggerManager, active matches are collected for
// each matching list entry. The id map is deliberately untouched until reset.
void TriggerableManager::removeTriggerable(Triggerable* trigger)
{
    std::list<std::list<Triggerable*>::iterator> removed;
    std::list<std::set<Triggerable*>::iterator> inactive;
    for (auto it = m_triggerables.begin(); it != m_triggerables.end(); ++it)
    {
        if (*it == trigger)
        {
            for (auto active = m_active.begin(); active != m_active.end(); ++active)
                if (*active == trigger)
                    inactive.push_back(active);
            removed.push_back(it);
        }
    }
    for (auto it = removed.begin(); it != removed.end(); ++it)
        m_triggerables.erase(*it);
    for (auto it = inactive.begin(); it != inactive.end(); ++it)
        m_active.erase(*it);
    m_nextTriggerable = m_triggerables.begin();
}

bool TriggerableManager::getTriggerablesWithTriggerId(int triggerId, std::vector<IObject*>& objects)
{
    for (auto it = m_triggerables.begin(); it != m_triggerables.end(); ++it)
    {
        Triggerable* trigger = *it;
        if (trigger->getTriggerId() == triggerId)
            objects.push_back(static_cast<IObject*>(trigger->queryInterface(IID_IObject)));
    }
    return !objects.empty();
}

Triggerable* TriggerableManager::getTriggerableFromId(int id)
{
    const auto it = m_byId.find(id);
    return it == m_byId.end() ? 0 : it->second;
}

void TriggerableManager::addTriggerableTarget(TriggerableTarget* target)
{
    m_targets.push_back(target);
}

void TriggerableManager::addTriggerableTargetTemplate(TriggerableTargetTemplate* objectTemplate)
{
    m_targetTemplates.push_back(objectTemplate);
}

// ELF81C4890: the two removal lists are never populated and the argument is
// unused. This is a shipped defect, not a reconstruction placeholder.
void TriggerableManager::removeTriggerableTarget(TriggerableTarget*)
{
    std::list<std::list<TriggerableTarget*>::iterator> removed;
    std::list<std::set<TriggerableTarget*>::iterator> inactive;
    for (auto it = removed.begin(); it != removed.end(); ++it)
        m_targets.erase(*it);
    for (auto it = inactive.begin(); it != inactive.end(); ++it)
        m_activeTargets.erase(*it);
    m_nextTarget = m_targets.begin();
}

// ELF81C49E0 / PE545060. By-value argument and result of getName; case sensitive.
TriggerableTarget* TriggerableManager::getTriggerableTargetByName(std::string name)
{
    for (auto it = m_targets.begin(); it != m_targets.end(); ++it)
    {
        TriggerableTarget* target = *it;
        if (target->getName() == name)
            return target;
    }
    return 0;
}

void TriggerableManager::reset()
{
    m_triggerables.clear();
    m_templates.clear();
    m_targets.clear();
    m_targetTemplates.clear();
    m_byId.clear();
    m_active.clear();
    m_activeTargets.clear();
    m_nextTriggerable = m_triggerables.begin();
    m_nextTarget = m_targets.begin();
}

void TriggerableManager::refresh()
{
    m_active.clear();
    for (auto it = m_triggerables.begin(); it != m_triggerables.end(); ++it)
        (*it)->refresh();
}

// Narrow concrete accessors used by the manager. Other runtime methods remain
// named declaration-only frontiers; no runtime factory is supplied here.
// ELF81C5B50/81C5B70, inlined in PE546450/5462B0/545680.
int Triggerable::getTriggerId() const { return m_template->m_triggerId; }
int Triggerable::getTypes() const { return m_template->m_types; }
// ELF81C7F00; PE545060 copies this member inline before operator==.
std::string TriggerableTarget::getName() { return m_name; }

} } }
