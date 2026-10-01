
// Owns the trigger tree for an animation-system template.


#ifndef ANIMATION_BONEANIMATION_TRIGGERMANAGER_H     // [INFERRED] guard style
#define ANIMATION_BONEANIMATION_TRIGGERMANAGER_H

#include <map>
#include <string>
#include <sstream>

#include "../../IO/File/IFileSystem.h"     // hfe::NoCaseStringCompare - the map's comparator
#include "Trigger.h"                       // the real hierarchy - Trigger::TriggerType is nested

namespace dice {

namespace hfe { namespace io { class IStream; } }

namespace anim {

class AnimationSystemTemplate;
class Bundle;


// Assigns priorities recursively, advancing counter as triggers are visited.

void setPriosForTrigger(Trigger* trigger, int& counter);

// Saves children first, then the trigger if unsaved.
void saveTrigger(Trigger* trigger, hfe::io::IStream* stream);

class TriggerManager
{
public:
    explicit TriggerManager(AnimationSystemTemplate* owner);
    ~TriggerManager();

    // the one method loadScript calls, and only when the console run succeeded.
    void setPriosForTriggers();

    // the trigger half of an animation script.
    Trigger* createTrigger(const std::string& type,
                           const std::string& name);
    Trigger* getTriggerByName(const std::string& name, bool unused);
    const std::string& getName(const Trigger* trigger) const;
    void setActiveTrigger(const std::string& name, Trigger* t);
    const std::string& getActiveTrigger() const;
    Trigger* getActiveTriggerPtr(Trigger::TriggerType type);
    void deleteTrigger(Trigger* trigger);
    void deleteActiveTrigger();
    void removeBundleFromAllTriggers(Bundle* bundle);

    void makeScript(hfe::io::IStream* stream);
    static std::string listTriggerTypes();
    std::string listTriggers() const;
    // [UNRESOLVED] getTriggers takes its vector by value and remains undeclared.

private:
    void listTriggers(Trigger* trigger, std::stringstream& output, int indent) const;
    // Byte offsets for the x86 layout.
    /* +0x00 */ std::map<std::string, Trigger*, hfe::NoCaseStringCompare> m_triggers;
    /* +0x0C */ Trigger* m_activeTrigger;                         // ctor 0
    /* +0x10 */ AnimationSystemTemplate* m_owner;                 // ctor = the constructor argument
};

} // namespace anim
} // namespace dice

#endif // ANIMATION_BONEANIMATION_TRIGGERMANAGER_H
