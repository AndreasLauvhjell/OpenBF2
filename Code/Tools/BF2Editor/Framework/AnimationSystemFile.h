// Edits animation-system .inc text in place, preserving untouched bytes without the engine.
// Each create command selects the owner of subsequent property lines; new lines follow writer ordering.
// References require earlier definitions: createdBefore checks this, and addObject's before argument
// places children/bundles before the blocks that reference them.
#ifndef BF2EDITOR_FRAMEWORK_ANIMATIONSYSTEMFILE_H
#define BF2EDITOR_FRAMEWORK_ANIMATIONSYSTEMFILE_H

#include "Framework/EditorConfig.h"

#include <string>
#include <utility>
#include <vector>

namespace bf2editor
{

enum class AnimationKind
{
    Animation,      // animationSystem.createAnimation <path>    -> animationManager.<p>
    Bundle,         // animationSystem.createBundle <name>       -> animationBundle.<p>
    Trigger,        // animationSystem.createTrigger <type> <n>  -> animationTrigger.<p>
    ValueHolder     // AnimationSystem.createValueHolder <name>  -> AnimationValueHolder.<p>
};

struct AnimationObject
{
    AnimationKind kind;
    std::string   type;         // a trigger's type ("Trigger", "MovementTrigger" ...); "" for the others
    std::string   name;         // a bundle's, trigger's or holder's name; an animation's .baf path
    size_t        createLine;   // the create line's index
    size_t        lastLine;     // its last owned line's index
};

struct AnimationProperty
{
    std::string property;       // after the kind's prefix: "fadeOutTime", "addAnimation", "setEventStartTime"
    std::string value;
    size_t      line;
};

struct BundleItem               // an animation of a bundle, or one of its events
{
    std::string name;           // the .baf path, or the event's name
    float       start;          // setAnimationStartTime / setEventStartTime; 0 when the file sets none
    bool        startSet;
    float       length;         // setAnimationLength (animations only); 0 when the file sets none
    bool        lengthSet;
};

class AnimationSystemFile
{
public:
    void parse(const std::string& text);
    std::string render() const;
    bool hasChanges() const { return m_changed; }
    size_t lineCount() const { return m_lines.size(); }
    const std::string& lineText(size_t index) const { return m_lines[index].text; }

    const std::vector<AnimationObject>& objects() const { return m_objects; }
    const AnimationObject* find(AnimationKind kind, const std::string& name) const;     // the last of that name, no case
    std::vector<AnimationProperty> properties(AnimationKind kind, const std::string& name) const;
    // The system's own lines (animationSystem.cameraSpring.*), property without "animationSystem.".
    std::vector<AnimationProperty> systemLines() const;

    // A property of an object set in place - its `which`-th line of that property, or the only one - or added
    // where the writer puts it in the object.
    bool setProperty(AnimationKind kind, const std::string& name, const std::string& property, const std::string& value,
                     int which, std::string& error);
    bool removeProperty(AnimationKind kind, const std::string& name, const std::string& property, int which,
                        std::string& error);
    bool setSystemProperty(const std::string& property, const std::string& value, std::string& error);
    bool removeSystemProperty(const std::string& property, std::string& error);
    // A line that adds (addAnimation, addEvent, addChild, addBundle ...): always a new line, where the writer
    // puts it in the object.
    bool addLine(AnimationKind kind, const std::string& name, const std::string& property, const std::string& value,
                 std::string& error);

    // A bundle's animations and events, in file order, with their start times (and the animations' lengths).
    std::vector<BundleItem> bundleAnimations(const std::string& bundle) const;
    std::vector<BundleItem> bundleEvents(const std::string& bundle) const;
    // Their start times and lengths: the set line rewritten, or added after the item's add line.
    bool setAnimationStart(const std::string& bundle, const std::string& path, float start, std::string& error);
    bool setEventStart(const std::string& bundle, const std::string& event, float start, std::string& error);
    // A length of 0 or less is refused: the engine divides by it (BoneAnimation::setLength, census §0.8).
    bool setAnimationLength(const std::string& bundle, const std::string& path, float length, std::string& error);
    // An animation or event taken out of a bundle with its start-time (and length) line.
    bool removeBundleItem(const std::string& bundle, const std::string& name, bool event, std::string& error);

    // A trigger's children and bundles, in file order; the triggers no trigger names as a child.
    std::vector<std::string> triggerChildren(const std::string& trigger) const;
    std::vector<std::string> triggerBundles(const std::string& trigger) const;
    std::vector<std::string> triggerRoots() const;
    // A trigger and every trigger under it (its children, followed), as TriggerManager::deleteTrigger takes them.
    std::vector<std::string> triggerSubtree(const std::string& trigger) const;
    // True when `name`'s create line comes before line `at`, so a reference there finds it at load.
    bool createdBefore(AnimationKind kind, const std::string& name, size_t at) const;
    // The line a new `property` line of the object would take (the writer's order).
    size_t insertionLine(AnimationKind kind, const std::string& name, const std::string& property) const;
    // Every line of `property` naming `value` (no case) in the blocks of `ownerKind` taken out; how many.
    int removeReferences(AnimationKind ownerKind, const std::string& property, const std::string& value);

    // A new object where the writer puts it: animations, bundles and value holders in name order, a trigger after
    // the last trigger, with a blank line between bundle and trigger blocks - or, when that place falls after
    // line `before`, right above line `before`, so the block that will name it finds it at load.
    bool addObject(AnimationKind kind, const std::string& type, const std::string& name, std::string& error,
                   size_t before = static_cast<size_t>(-1));
    // An object taken out: its create line and the lines it owns, and a bundle's or trigger's separator line.
    bool removeObject(AnimationKind kind, const std::string& name, std::string& error);

    static const char* createCommand(AnimationKind kind);
    static const char* prefix(AnimationKind kind);
    static const char* kindName(AnimationKind kind);                // "animation", "bundle", "trigger", "valueholder"
    static bool kindFromName(const std::string& name, AnimationKind& kind);
    static std::string number(float value);     // %g, as the stock files write times

private:
    struct Line
    {
        std::string text;
        std::string end;
    };
    void index();
    std::string newline() const;
    bool insertAt(size_t position, const std::string& text);
    bool insertAfter(size_t at, const std::string& text) { return insertAt(at + 1, text); }
    int ownerOf(const AnimationObject* o) const { return static_cast<int>(o - &m_objects[0]); }

    std::vector<Line>            m_lines;
    std::vector<AnimationObject> m_objects;
    std::vector<int>             m_owner;       // per line: the object it belongs to, -1 for none
    bool                         m_changed = false;
};

} // namespace bf2editor

#endif // BF2EDITOR_FRAMEWORK_ANIMATIONSYSTEMFILE_H
