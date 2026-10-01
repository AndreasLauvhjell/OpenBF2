
// Animation templates load in two phases: registration followed by script and animation loading on core event 3.


#ifndef ANIMATION_BONEANIMATION_ANIMATIONSYSTEM_H     // [INFERRED] guard style
#define ANIMATION_BONEANIMATION_ANIMATIONSYSTEM_H

#include <list>
#include <map>
#include <string>

// NoCaseStringCompare lives in IFileSystem.h, not in StringUtil.h - see the note on it there.
#include "../../IO/File/IFileSystem.h"     // hfe::NoCaseStringCompare - the map's comparator
#include "../../Game/IEventListener.h"    // the manager's second base, at +0x04
#include "../../Math/Vec3.h"              // AnimationSystem's two movement vectors

namespace dice {

namespace hfe { namespace world { class IObject; } }

namespace anim {

// TriggerManager and BundleManager are recovered - TriggerManager.h and Bundle.h.
// They are still forward-declared here rather than included: this header only stores pointers to
// them, and their own headers include this one for AnimationSystemTemplate.
class TriggerManager;          // AnimationSystemTemplate +0x04, owned
class BundleManager;           // AnimationSystemTemplate +0x08, owned
class BundlePlayer;
class Skeleton;
class Trigger;
class CameraSpring;            // 80 bytes - CameraSpring.h; forward-declared, only stored here
// AnimationSystem is recovered - defined at the bottom of this header, where it
// belongs: it lives in the same TU as the two classes above it. Forward-declared here because
// IAnimationSystemTemplate::makeInstance returns one.
class AnimationSystem;
class Bundle;                  // Bundle.h; only pointers here


// Reference-counted interface independent of IBase. Destruction precedes addRef/release.

class IAnimationSystemTemplate
{
public:
    virtual ~IAnimationSystemTemplate() {}                        // +0x00 +0x04
    virtual void addRef() = 0;                                    // +0x08
    virtual int  release() = 0;                                   // +0x0C
    virtual bool loadScript(const std::string& path) = 0;         // +0x10
    virtual AnimationSystem* makeInstance() = 0;                  // +0x14
    virtual const std::string& getPath() const = 0;               // +0x18
    virtual CameraSpring* getCameraSpring() const = 0;            // +0x1C
};


// AnimationSystemTemplate - 24 bytes (operator new(0x18) in the manager's load).

class AnimationSystemTemplate : public IAnimationSystemTemplate
{
public:
    AnimationSystemTemplate();
    virtual ~AnimationSystemTemplate();

    virtual void addRef();
    virtual int  release();
    // , and it is what actually reads the .inc. Deferred to the ECCore
    // event; see the .cpp for why calling it twice is a no-op and what that costs reloadScript.
    virtual bool loadScript(const std::string& path);
    virtual AnimationSystem* makeInstance();
    // it asks the manager. There is no path field - getPath forwards to the manager's +0x1C,
    // which finds this template in the map and returns its KEY.
    virtual const std::string& getPath() const;
    virtual CameraSpring* getCameraSpring() const;

    // inline, and the evidence is the same as for ValueHolderManager::setActive: ModuleBone-
    // Animation's file-local getActiveBundleManager()/getActiveTriggerManager() read +0x08 and
    // +0x04 straight out of this object from another translation unit, with no call. That is
    // what an accessor defined in a header compiles to. [CHOSEN] names.
    BundleManager*  getBundleManager() const  { return m_bundleManager; }
    TriggerManager* getTriggerManager() const { return m_triggerManager; }

    void setUseCameraSpring(bool use);
    bool getUseCameraSpring() const;
    void saveActive();
    void save(const std::string& path);
    // shipped defect - it cannot reload anything. See the .cpp.
    bool reloadScript();

    // Global animation time scale, initially 1.0f.
    static float globalTimeModifier;

private:
    // The manager's deferred-load pass accesses the bundle manager directly.
    friend class AnimationSystemTemplateManager;

    /* +0x04 */ TriggerManager* m_triggerManager;   // ctor 0, owned - the dtor destroys and frees it
    /* +0x08 */ BundleManager*  m_bundleManager;    // ctor 0, owned - likewise. The manager's
                                                    // Owned spring storage is freed without calling its destructor, matching the original lifecycle.
    /* +0x0C */ CameraSpring*   m_cameraSpring;     // ctor 0, owned - 80 bytes
    // zero, not one. load() addRefs before handing the template back, so the count is 1 by the
    // time a caller sees it. and the failed-load path does not release - it deletes outright
    // (vtable +0x04), so the count never goes negative. An earlier reading of this file claimed
    // it did; that claim came from mistaking the deleting destructor slot for release().
    /* +0x10 */ int             m_refCount;         // ctor 0
    /* +0x14 */ int             m_dword14;          // [UNRESOLVED] and not written by the ctor
};


// Manager interface independent of IBase; destruction precedes its own methods.

class IAnimationSystemTemplateManager
{
public:
    virtual ~IAnimationSystemTemplateManager() {}                              // +0x00 +0x04
    virtual void create(const std::string& path) = 0;                          // +0x08
    virtual AnimationSystemTemplate* load(const std::string& name) = 0;        // +0x0C
    virtual int  release(IAnimationSystemTemplate* templ) = 0;                 // +0x10
    virtual void setActiveSystem(const std::string& name) = 0;                 // +0x14
    virtual const std::string& getActiveSystem() const = 0;                    // +0x18
    virtual const std::string& getPath(const IAnimationSystemTemplate*) const = 0;   // +0x1C
    // the one inconsistency in this interface: setActive takes the concrete type where release
    // and getPath take the interface. dice's, kept.
    virtual void setActive(AnimationSystemTemplate* templ) = 0;                // +0x20
    virtual AnimationSystemTemplate* getActive() const = 0;                    // +0x24
    virtual void hide() = 0;                                                   // +0x28
    virtual std::string list() = 0;                                           // +0x2C
    virtual void setPlaybackModifier(float modifier) = 0;                      // +0x30
    virtual float getPlaybackModifier() const = 0;                             // +0x34
};


// AnimationSystemTemplateManager - 32 bytes (operator new(0x20) in AnimModule::init).
// Second base: hfe::IEventListener at +0x04.

class AnimationSystemTemplateManager : public IAnimationSystemTemplateManager,
                                       public hfe::IEventListener
{
public:
    AnimationSystemTemplateManager();
    // it deletes every registered template outright, refcount or not - the walk calls each
    // template's vtable +0x04, the deleting destructor, not release().
    virtual ~AnimationSystemTemplateManager();

    virtual void create(const std::string& path);
    virtual AnimationSystemTemplate* load(const std::string& name);
    virtual int  release(IAnimationSystemTemplate* templ);
    virtual void setActiveSystem(const std::string& name);
    virtual const std::string& getActiveSystem() const;
    virtual const std::string& getPath(const IAnimationSystemTemplate*) const;
    virtual void setActive(AnimationSystemTemplate* templ);
    virtual AnimationSystemTemplate* getActive() const;
    virtual void hide();
    virtual std::string list();
    virtual void setPlaybackModifier(float modifier);
    virtual float getPlaybackModifier() const;

    // the deferred load itself. ECCore, event id 3.
    virtual void handleEvent(hfe::EventCategory category, unsigned int id,
                             hfe::EventNode* node, void* data);

private:
    // the defer switch, and the single most load-bearing byte in this class. true from the
    // constructor. While it is set, load() registers a template without reading its file and the
    // manager listens for the ECCore event that reads them all in one pass. Nothing recovered
    // clears it. [CHOSEN] name.
    /* +0x08 */ bool m_deferLoading;
    /* +0x0C */ std::map<std::string, AnimationSystemTemplate*, hfe::NoCaseStringCompare> m_templates;
    /* +0x18 */ AnimationSystemTemplate* m_active;                // ctor 0
    /* +0x1C */ float m_playbackModifier;                         // ctor 1.0f
};


// Animation playback interface shared by first- and third-person systems. It has no IBase reference-count quartet.

class IAnimationSystem
{
public:
    virtual ~IAnimationSystem() {}                                            // +0x00 +0x04
    virtual AnimationSystemTemplate* getTemplate() = 0;                       // +0x08
    virtual void reset() = 0;                                                 // +0x0C
    virtual void update(hfe::world::IObject* object, float dt) = 0;           // +0x10
    virtual void applyOnSkeleton(Skeleton* skeleton, bool postTrigger) = 0;   // +0x14
    virtual void disable(bool disable) = 0;                                   // +0x18
    virtual bool playTrigger(const std::string& name, bool exclusive) = 0;    // +0x1C
    virtual bool playPostTrigger(const std::string& name, bool exclusive) = 0;// +0x20
    virtual void setActiveTrigger(const std::string& name, bool lock) = 0;    // +0x24
    virtual void copyStateToSystem(IAnimationSystem* other, bool b) = 0;      // +0x28
    virtual void setTriggerPose(unsigned int pose) = 0;                       // +0x2C
    virtual void setTriggerMessage(unsigned int mask) = 0;                    // +0x30
    virtual void setTriggerOnceMessage(unsigned int mask) = 0;                // +0x34
    virtual void clearTriggerMessage(unsigned int mask) = 0;                  // +0x38
    virtual void setTriggerMovement(float a, const hfe::Vec3& v,
                                    float b, const hfe::Vec3& w) = 0;         // +0x3C
    virtual void getPlayingAnimationLength(float& length, float& time,
                                          bool movementOnly) = 0;              // +0x40
    virtual void syncMovementAnimations(const IAnimationSystem* other) = 0;   // +0x44
    virtual void setForceTime(float t) = 0;                                   // +0x48
    virtual float getForceTime() = 0;                                         // +0x4C
};


// Runtime playback state for one animation rig. Owns the active bundle players.

class AnimationSystem : public IAnimationSystem
{
public:
    explicit AnimationSystem(AnimationSystemTemplate& templ);
    virtual ~AnimationSystem();

    // called after construction, and it does exactly one interesting thing: it looks up a trigger
    // literally named "postRoot". that name appears twice in the entire shipped content SET -
    // `createTrigger Trigger postRoot`, in two .inc files - so it is a rare, deliberate hook rather
    // than a convention every animation system follows.
    void init();

    virtual AnimationSystemTemplate* getTemplate();
    virtual void reset();
    // The bool selects the post-trigger players, not a rig - see the .cpp.
    virtual void applyOnSkeleton(Skeleton* skeleton, bool postTrigger);
    virtual void disable(bool disable);
    virtual bool playTrigger(const std::string& name, bool exclusive);
    virtual bool playPostTrigger(const std::string& name, bool exclusive);
    virtual void setActiveTrigger(const std::string& name, bool lock);
    virtual void setTriggerMessage(unsigned int mask);
    virtual void setTriggerOnceMessage(unsigned int mask);
    virtual void clearTriggerMessage(unsigned int mask);
    virtual void setForceTime(float t);
    virtual float getForceTime();

    BundlePlayer* createBundlePlayer(Bundle* bundle, Trigger* trigger);
    void releaseBundlePlayer(BundlePlayer* player);
    void stopAllPlaying();
    void setFadeInTime(Trigger* trigger);
    float getIdleTime();

    // the one playback query, and the only body in the runtime half that reads a BundlePlayer's
    // fields rather than just its pointer. It walks m_players backwards - see the .cpp.
    virtual void getPlayingAnimationLength(float& length, float& time,
                                           bool movementOnly);

    // The playback itself - all six (the animation-playback sitting)
    // update - the PER-frame step. See the .cpp for the full walk: the pending disable, the
    // "startup"/"root" trigger lookups, the message masks, the trigger updates, the fade-in
    // envelope, every running BundlePlayer's update, and the two idle timers.
    virtual void update(hfe::world::IObject* object, float dt);
    // Copies the other system's two message masks into this one, then pushes this one's pose
    // (when asked) and movement into the other - the 1P/3P hand-over.
    virtual void copyStateToSystem(IAnimationSystem* other, bool copyPose);
    // The pose transitions: entering prone picks one of five "...ToProne" triggers by the
    // current movement, leaving prone plays "fromProne"; either stops everything first.
    virtual void setTriggerPose(unsigned int pose);
    // (dt, direction, speed, look). A non-positive dt stores the three verbatim; otherwise the
    // direction is smoothed toward the new one unless a dominant axis flipped sign, and the
    // (old - new) velocity over dt becomes m_movementRate.
    virtual void setTriggerMovement(float dt, const hfe::Vec3& direction,
                                    float speed, const hfe::Vec3& look);
    // Pairs this system's looping movement players with the other's, in order, copying the
    // other's time onto ours - the two rigs of one soldier keep their walk cycles in phase.
    virtual void syncMovementAnimations(const IAnimationSystem* other);
    // Find-or-create the player for the bundle, keeping the list sorted by priority
    // ascending; then set its parameters, start it, and stamp the priority and the post flag.
    // A priority above 9000 stops everything else first.
    BundlePlayer* playBundle(Trigger* trigger, Bundle* bundle, int prio,
                             float parameter, float speed);

private:
    // The trigger family reads and writes the fields below from Trigger.cpp with no accessor
    // call anywhere in the binary (the message masks, the movement group, the phase). A friend
    // line per class is the narrowest way to reproduce that access without widening the public
    // surface - the same argument as ObjectLayoutAccess.h's.
    friend class Trigger;
    friend class PoseTrigger;
    friend class MessageTrigger;
    friend class SwitchMessageTrigger;
    friend class MovementTrigger;
    friend class ForwardTrigger;
    friend class SideTrigger;
    friend class UpTrigger;
    friend class TurnTrigger;
    friend class LookAroundTrigger;

    // by setTriggerPose (its only writer) and PoseTrigger::update (which
    // indexes its children by it): the current pose. Named after the setter.
    /* +0x04 */ unsigned int m_triggerPose;    // ctor 0
    // the two message masks. setTriggerMessage ORs into +0x08 and clearTriggerMessage clears bits
    // from it; setTriggerOnceMessage ORs into +0x10. update() consumes the once-mask: or-ed
    // into +0x08 for the trigger walk, then cleared from both.
    /* +0x08 */ unsigned int m_triggerMessages;
    // update() snapshots m_triggerMessages here at the end of every trigger
    // walk and compares against it at the start of the next - last frame'S masks - and
    // SwitchMessageTrigger::update reads it to detect a message's edge. [CHOSEN] name.
    /* +0x0C */ unsigned int m_lastTriggerMessages;   // ctor 0
    /* +0x10 */ unsigned int m_triggerOnceMessages;
    // Smoothed direction (side/up/forward), speed, and (old velocity - new velocity) / dt.
    // Look x/y drive look-around; look y also drives turning. [CHOSEN] Field names.
    /* +0x14 */ hfe::Vec3 m_movement;          // ctor 0
    /* +0x20 */ float     m_movementSpeed;     // ctor 0
    /* +0x24 */ hfe::Vec3 m_movementRate;      // ctor 0
    /* +0x30 */ hfe::Vec3 m_look;              // ctor 0
    /* +0x3C */ float m_idleTime;              // getIdleTime picks between these two
    /* +0x40 */ float m_postIdleTime;          // on the byte at +0x58
    // by MovementTrigger::applyAnimations: the phase of the movement cycle
    // (player time / bundle length), seeded from the anim_startLeft/RightOffset cvars when a
    // side-step starts, and read back to keep a new cycle in step with the old. A float, not a
    // word. [CHOSEN] name.
    /* +0x44 */ float m_movementPhase;         // ctor 0
    /* +0x48 */ AnimationSystemTemplate* m_template;      // the lookup copy - see the class note
    /* +0x4C */ AnimationSystemTemplate* m_templateAgain; // what getTemplate() returns
    /* +0x50 */ Trigger* m_activeTrigger;      // ctor 0; setActiveTrigger stores here; update()
                                               // looks up "root" while it is null
    /* +0x54 */ Trigger* m_postRootTrigger;    // init() looks it up by name
    /* +0x58 */ bool m_inPostTrigger;          // ctor false; playPostTrigger brackets its call with it
    // by update(): true means "the startup trigger has not run yet". The
    // first update after construction or reset clears it and plays the trigger named "startup"
    // when the template has one. [CHOSEN] name.
    /* +0x59 */ bool m_startupPending;         // ctor true, reset true
    /* +0x5A */ bool m_locked;                 // ctor false - setActiveTrigger/playTrigger refuse when set
    /* +0x5B */ bool m_playing;                // ctor false; playTrigger sets it, update() clears it
                                               // when a player fades out or is dropped
    /* +0x5C */ std::list<BundlePlayer*> m_players;   // the running bundles, priority ascending
    /* +0x60 */ int   m_dword60;               // not written by the constructor. [UNRESOLVED]
    /* +0x64 */ float m_fadeInTime;            // setFadeInTime copies it off the trigger; update()
                                               // runs it down and derives m_weight from it
    /* +0x68 */ float m_weight;                // ctor 1.0f - applyOnSkeleton writes it into the
                                               // skeleton's +0x00, the skeleton-wide animation weight
    /* +0x6C */ Trigger* m_fadeInFrom;         // the trigger m_fadeInTime was taken from
    // update() calls init() while this is set - the lazy first-update
    // initialisation. [CHOSEN] name.
    /* +0x70 */ bool m_needsInit;              // ctor true, init() clears it
    /* +0x74 */ float m_forceTime;             // not written by the constructor - the flag guards it
    /* +0x78 */ bool m_forceTimeSet;           // ctor false; setForceTime raises it
    // Negative means no pending reenable; disable(true) sets exact time + 0.3 seconds.
    /* +0x7C */ double m_reenableTime;         // ctor -1.0
};


// Created directly by AnimModule::init.
extern AnimationSystemTemplateManager* animationSystemTemplateManager;

} // namespace anim
} // namespace dice

#endif // ANIMATION_BONEANIMATION_ANIMATIONSYSTEM_H
