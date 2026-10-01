// [INFERRED] Source placement.

// Animation playback unit containing animations and events sorted by start time.


#ifndef ANIMATION_BONEANIMATION_BUNDLE_H     // [INFERRED] guard style
#define ANIMATION_BONEANIMATION_BUNDLE_H

#include <list>
#include <map>
#include <string>
#include <utility>

#include "../../IO/File/IFileSystem.h"     // hfe::NoCaseStringCompare - the map's comparator

namespace dice {

namespace hfe { namespace io { class IStream; } }

namespace anim {

class AnimationSystemTemplate;
class BoneAnimation;
class BundleManager;

class Bundle
{
public:

    // Owns named bundles for an animation-system template.

    class BundleEvent
    {
    public:
        explicit BundleEvent(const std::string& name);

        /* +0x00 */ std::string m_name;    // lowercased, prefix and all
        /* +0x04 */ int m_id;              // the number after the colon
        /* +0x08 */ int m_type;            // 0 = none, 1 = sound, 2 = effect. [CHOSEN] names
    };

    typedef std::list<std::pair<float, BoneAnimation*> > AnimationList;
    typedef std::list<std::pair<float, BundleEvent> >    EventList;

    explicit Bundle(BundleManager* manager);
    ~Bundle();

    // the deferred load'S last step, per bundle - BundleManager::postLoad calls it on every
    // bundle after every animation has been read.
    void init();
    void adjustLength();

    const std::string& getName() const;

    BoneAnimation* getAnimation(int index) const;
    AnimationList::iterator getAnimation(const std::string& name);

    void  addAnimation(const std::string& name);
    void  removeAnimation(const std::string& name);
    void  setAnimationStartTime(const std::string& name, float t);
    float getAnimationStartTime(const std::string& name);
    void  setAnimationLength(const std::string& name, float len);
    float getAnimationLength(const std::string& name);

    void addEvent(const std::string& name);

    void makeScript(hfe::io::IStream* stream);
    std::string listAnimations() const;
    std::string listEvents() const;
    void getAnimations(std::map<std::string, BoneAnimation*,
                                hfe::NoCaseStringCompare>& out) const;

    // the event half, completed.
    void  removeEvent(const std::string& name);
    void  setEventStartTime(const std::string& name, float t);
    float getEventStartTime(const std::string& name);
    // [CHOSEN, not recovered] Shared case-insensitive event lookup used by the three event operations.
    EventList::iterator findEvent(const std::string& name);

    // Public for the same reason CameraSpring's and ValueHolder's fields are: ModuleBoneAnimation's
    // executors store straight into these offsets with no accessor call anywhere in the binary.
    /* +0x00 */ AnimationList m_animations;   // sorted by start time
    /* +0x04 */ EventList     m_events;
    /* +0x08 */ BundleManager* m_manager;
    /* +0x0C */ float length;                 // ctor 1.0f   (ConsoleClass197)
    /* +0x10 */ float fadeInTime;             // ctor 0.2f   (ConsoleClass204)
    /* +0x14 */ float fadeOutTime;            // ctor 0.1f   (ConsoleClass205)
    /* +0x18 */ bool jumpToLastAnimationAtStop;   // ctor false (cc199)
    /* +0x19 */ bool useSpeedAsTime;              // ctor false (cc203)
    /* +0x1A */ bool abruptPlayback;              // ctor false (cc202)
    /* +0x1B */ bool isLooping;                   // ctor true (cc201)
    /* +0x1C */ bool playBackward;                // ctor false (cc198)
    /* +0x1D */ bool playForever;                 // ctor false (cc200)
    // not console-exposed, and that is the clue to what it is: init() sets it when any animation
    // in the bundle is itself looping. A derived flag, recomputed on every structural change, which
    // is why no .inc can set it. [CHOSEN] name.
    /* +0x1E */ bool m_hasLoopingAnimation;       // ctor false
};


// dice::anim::BundleManager - 20 bytes, the second of the three managers an AnimationSystemTemplate
// owns. Byte for byte the same shape as TriggerManager: a case-insensitive map from name to object,
// an "active" pointer at +0x0C, the owning template at +0x10.

class BundleManager
{
public:
    explicit BundleManager(AnimationSystemTemplate* owner);
    ~BundleManager();

    void postLoad();

    Bundle* createBundle(const std::string& name);
    void deleteBundle(Bundle* bundle);
    void deleteActiveBundle();
    void setActiveBundle(const std::string& name);
    Bundle* getActiveBundlePtr() const;
    const std::string& getActiveBundle() const;
    Bundle* getBundleByName(const std::string& name);
    const std::string& getBundleName(const Bundle* bundle) const;

    void makeScript(hfe::io::IStream* stream);
    std::string listBundles() const;
    // [UNRESOLVED] getBundles takes its vector by value; no guessed output-reference API is exposed.

private:
    // Byte offsets for the x86 layout.
    /* +0x00 */ std::map<std::string, Bundle*, hfe::NoCaseStringCompare> m_bundles;
    /* +0x0C */ Bundle* m_activeBundle;                           // ctor 0
    /* +0x10 */ AnimationSystemTemplate* m_owner;                 // ctor = the constructor argument
};

} // namespace anim
} // namespace dice

#endif // ANIMATION_BONEANIMATION_BUNDLE_H
