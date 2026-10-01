// [INFERRED] beside Trigger.cpp. ELF 81C37D0..81C4C7A; PE ctor 546370.
#ifndef GAME_OBJECTS_GAME_TRIGGERABLEMANAGER_H
#define GAME_OBJECTS_GAME_TRIGGERABLEMANAGER_H

#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace dice { namespace hfe { namespace world {
class IObject;
class Triggerable;
class TriggerableTemplate;
class TriggerableTarget;
class TriggerableTargetTemplate;

// No base. ELF ZTV88A2AA0 contains only D1/D0; handleUpdate is NON-virtual.
// Native containers retain the source operations, not either retail STL layout.
class TriggerableManager
{
public:
    TriggerableManager();
    virtual ~TriggerableManager();
    void handleUpdate(float deltaTime, unsigned int tick);
    bool hasActivePartner(Triggerable* trigger);
    void activateTrigger(Triggerable* trigger);
    bool alreadyActive(Triggerable* trigger);
    void addTriggerable(Triggerable* trigger);
    void addTriggerableTemplate(TriggerableTemplate* objectTemplate);
    void removeTriggerable(Triggerable* trigger);
    bool getTriggerablesWithTriggerId(int triggerId, std::vector<IObject*>& objects);
    Triggerable* getTriggerableFromId(int id);
    void addTriggerableTarget(TriggerableTarget* target);
    void addTriggerableTargetTemplate(TriggerableTargetTemplate* objectTemplate);
    void removeTriggerableTarget(TriggerableTarget* target);
    TriggerableTarget* getTriggerableTargetByName(std::string name);
    void reset();
    void refresh();

private:
    std::list<Triggerable*> m_triggerables;                        // ELF +04 / PE +04
    std::list<TriggerableTemplate*> m_templates;                   // ELF +08 / PE +10
    std::list<TriggerableTarget*> m_targets;                       // ELF +0C / PE +1C
    std::list<TriggerableTargetTemplate*> m_targetTemplates;       // ELF +10 / PE +28
    std::map<int, Triggerable*> m_byId;                            // ELF +14 / PE +34
    std::list<Triggerable*>::iterator m_nextTriggerable;           // ELF +20 / PE +40
    std::set<Triggerable*> m_active;                              // ELF +24 / PE +44
    std::list<TriggerableTarget*>::iterator m_nextTarget;          // ELF +30 / PE +50
    std::set<TriggerableTarget*> m_activeTargets;                  // ELF +34 / PE +54
};
extern TriggerableManager* triggerableManager;
} } }
#endif
