// Trigger creation, lookup, ownership, and script loading.

#include "TriggerManager.h"

#include "Trigger.h"

#include "AnimationSystem.h"
#include "../../Core/Debug.h"
#include "../../IO/Stream/IStream.h"
#include <cstring>
#include <sstream>

namespace dice {
namespace anim {


// Assigns increasing priorities depth-first, preincrementing counter for each trigger.
// Skips null children; requires an acyclic tree.

void setPriosForTrigger(Trigger* trigger, int& counter)
{
    if (!trigger)
        return;

    trigger->setPrio(++counter);

    const std::vector<Trigger*>& children = trigger->getChildren();
    for (std::vector<Trigger*>::const_iterator it = children.begin(); it != children.end(); ++it)
        setPriosForTrigger(*it, counter);
}


// Initializes trigger storage for the supplied template owner.

TriggerManager::TriggerManager(AnimationSystemTemplate* owner)
    : m_activeTrigger(0)
    , m_owner(owner)
{
}


// Deletes all owned triggers.

TriggerManager::~TriggerManager()
{
    for (std::map<std::string, Trigger*, hfe::NoCaseStringCompare>::iterator it = m_triggers.begin();
         it != m_triggers.end(); ++it)
    {
        if (it->second)
            delete it->second;                // vtable +0x04
    }
    m_triggers.clear();
}


// Creates a trigger of the requested type and name.

Trigger* TriggerManager::createTrigger(const std::string& type, const std::string& name)
{
    if (getTriggerByName(name, false))
        return 0;                                  // cursor left alone - see above

    Trigger* trigger = 0;
    if      (type.compare("Trigger") == 0)              trigger = new Trigger();               // 0x28
    else if (type.compare("PoseTrigger") == 0)          trigger = new PoseTrigger();           // 0x28
    else if (type.compare("IdleTrigger") == 0)          trigger = new IdleTrigger();           // 0x34
    else if (type.compare("RandomTrigger") == 0)        trigger = new RandomTrigger();         // 0x28
    else if (type.compare("MessageTrigger") == 0)       trigger = new MessageTrigger();        // 0x2C
    else if (type.compare("SwitchMessageTrigger") == 0) trigger = new SwitchMessageTrigger();  // 0x2C
    else if (type.compare("MovementTrigger") == 0)      trigger = new MovementTrigger();       // 0x30
    else if (type.compare("ForwardTrigger") == 0)       trigger = new ForwardTrigger();        // 0x30
    else if (type.compare("SideTrigger") == 0)          trigger = new SideTrigger();           // 0x30
    else if (type.compare("UpTrigger") == 0)            trigger = new UpTrigger();             // 0x30
    else if (type.compare("TurnTrigger") == 0)          trigger = new TurnTrigger();           // 0x30
    else if (type.compare("LookAroundTrigger") == 0)    trigger = new LookAroundTrigger();     // 0x30
    else
    {
        // DEBUGTYPE_6 is Error and Error aborts. See the block above.
        hfe::Debug(hfe::DEBUGTYPE_6, "Animation/BoneAnimation/TriggerManager.cpp", 125, "")
            << m_owner->getPath() << " " << type << " : type not found";
        return 0;
    }

    m_triggers.insert(std::make_pair(name, trigger));
    m_activeTrigger = trigger;
    trigger->activate();                           // vtable +0x24
    return trigger;
}

// A keyed find; a miss is a null.
// the second parameter is never read. Its mangling says the method takes (const std::string&,
// bool) and the 61-byte body does not touch it. Kept in the signature because the signature is
// public API; noted because a reader will look for the branch that is not there.
Trigger* TriggerManager::getTriggerByName(const std::string& name, bool /*unused*/)
{
    std::map<std::string, Trigger*, hfe::NoCaseStringCompare>::iterator it = m_triggers.find(name);
    return it == m_triggers.end() ? 0 : it->second;
}


// Returns the registered name, or the shared string "no trigger found".

const std::string& TriggerManager::getName(const Trigger* trigger) const
{
    static const std::string s_notFound("no trigger found");

    if (!trigger)
        return s_notFound;

    for (std::map<std::string, Trigger*, hfe::NoCaseStringCompare>::const_iterator it =
             m_triggers.begin(); it != m_triggers.end(); ++it)
    {
        if (it->second == trigger)
            return it->first;
    }
    return s_notFound;
}


// Selects a trigger by pointer or name and activates it. A failed name lookup clears the selection.

void TriggerManager::setActiveTrigger(const std::string& name, Trigger* trigger)
{
    m_activeTrigger = trigger ? trigger : getTriggerByName(name, false);
    if (m_activeTrigger)
        m_activeTrigger->activate();               // vtable +0x24
}

// The round trip.
const std::string& TriggerManager::getActiveTrigger() const
{
    return getName(m_activeTrigger);
}


// Returns the active trigger if its type matches, or null. TRIGGERTYPE_BASE accepts any active trigger.

Trigger* TriggerManager::getActiveTriggerPtr(Trigger::TriggerType type)
{
    if (!m_activeTrigger)
        return 0;
    if (type == Trigger::TRIGGERTYPE_BASE)
        return m_activeTrigger;                    // 0 means "any" - see above
    return m_activeTrigger->getType() == type ? m_activeTrigger : 0;
}


// Deletes a trigger subtree, removing children before their parent.

void TriggerManager::deleteTrigger(Trigger* trigger)
{
    if (!trigger)
        return;

    for (std::map<std::string, Trigger*, hfe::NoCaseStringCompare>::iterator it = m_triggers.begin();
         it != m_triggers.end(); ++it)
    {
        it->second->removeChildByPtr(trigger);     // vtable +0x38
    }

    while (!trigger->m_children.empty())
    {
        Trigger* child = trigger->m_children.front();
        trigger->removeChildByPtr(child);
        deleteTrigger(child);
    }

    m_triggers.erase(getName(trigger));
    delete trigger;                                // vtable +0x04
}

// The null-out happens after the delete and unconditionally.
void TriggerManager::deleteActiveTrigger()
{
    deleteTrigger(m_activeTrigger);
    m_activeTrigger = 0;
}

// The other half of BundleManager::deleteBundle - a bundle about to die has to
// leave every trigger that plays it, and this is the sweep that does it.
void TriggerManager::removeBundleFromAllTriggers(Bundle* bundle)
{
    for (std::map<std::string, Trigger*, hfe::NoCaseStringCompare>::iterator it = m_triggers.begin();
         it != m_triggers.end(); ++it)
    {
        it->second->removeBundleByPtr(bundle);     // vtable +0x48
    }
}


// Assigns traversal priorities after successful script loading.

void TriggerManager::setPriosForTriggers()
{
    int counter = 0;

    for (std::map<std::string, Trigger*, hfe::NoCaseStringCompare>::iterator it = m_triggers.begin();
         it != m_triggers.end(); ++it)
    {
        if (it->second->isRoot())
            setPriosForTrigger(it->second, counter);
    }
}

// . Recurse before testing already-saved. Shared descendants are
// emitted once, but this is not a cycle guard: the shipped recursion is unbounded.
void saveTrigger(Trigger* trigger, hfe::io::IStream* stream)
{
    if (!trigger) return;
    for (auto it = trigger->m_children.begin(); it != trigger->m_children.end(); ++it)
        saveTrigger(*it, stream);
    if (!trigger->getAlreadySaved())
    {
        trigger->makeScript(stream);
        stream->write("\n", 1);
    }
}

// . Reset every saved flag first, then save roots in map order.
void TriggerManager::makeScript(hfe::io::IStream* stream)
{
    const char* header = "\nrem *** Triggers ***\n";
    stream->write(header, static_cast<unsigned int>(std::strlen(header)));
    for (auto it = m_triggers.begin(); it != m_triggers.end(); ++it)
        it->second->setAlreadySaved(false);
    for (auto it = m_triggers.begin(); it != m_triggers.end(); ++it)
        if (!it->second->m_isChild) saveTrigger(it->second, stream);
}

// . static: neither caller supplies a manager; the sole argument
// is the hidden returned-string storage. The list is available with no active template.
std::string TriggerManager::listTriggerTypes()
{
    const char* types[] = { "Trigger", "PoseTrigger", "IdleTrigger", "RandomTrigger",
        "MessageTrigger", "SwitchMessageTrigger", "MovementTrigger", "ForwardTrigger",
        "SideTrigger", "UpTrigger", "TurnTrigger", "LookAroundTrigger", 0 };
    std::stringstream output;
    for (int i = 0; types[i]; ++i) output << types[i] << "\n";
    return output.str();
}

// . A null child is skipped; each depth adds exactly four spaces.
void TriggerManager::listTriggers(Trigger* trigger, std::stringstream& output, int indent) const
{
    if (!trigger) return;
    for (int i = 0; i < indent; ++i) output << " ";
    output << getName(trigger) << "\n";
    for (std::size_t i = 0; i < trigger->m_children.size(); ++i)
        listTriggers(trigger->m_children[i], output, indent + 4);
}

// . Only roots begin a listing, without a visited set.
std::string TriggerManager::listTriggers() const
{
    std::stringstream output;
    for (auto it = m_triggers.begin(); it != m_triggers.end(); ++it)
        if (!it->second->m_isChild) listTriggers(it->second, output, 0);
    return output.str();
}

} // namespace anim
} // namespace dice
