
// One running bundle/trigger pair. Tracks time, events, fading and animation staging.


#ifndef ANIMATION_BONEANIMATION_BUNDLEPLAYER_H     // [INFERRED] guard style
#define ANIMATION_BONEANIMATION_BUNDLEPLAYER_H

#include "Bundle.h"

namespace dice {
namespace hfe { namespace world { class IObject; } }
namespace anim {

class Trigger;
class Skeleton;
class BoneAnimation;

class BundlePlayer
{
public:
    BundlePlayer(Bundle& bundle, Trigger& trigger);
    ~BundlePlayer();                                              // (D1),  5 B - empty

    // Rewinds everything but the priority, the force time and m_blendIndexB. Called by the
    // constructor; nothing else recovered calls it.
    void resetTime();

    // Names the two animations a movement trigger cross-fades and how far between them we are.
    // Only acts on a bundle with more than one animation. A change of pairing while the bundle has
    // a fade-in time restarts m_blendTransition at zero.
    void setBlendAnimations(float blend, int indexA, int indexB);
    // (parameter, speed). A non-negative parameter on a looping bundle becomes the parameter
    // weight; jumpToLastAnimationAtStop forces speed 1.0; a negative speed on a one-shot bundle
    // that is not yet playing starts it from the end.
    void setParameters(float parameter, float speed);
    float getSlerpFactor() const;
    // Restarting an abruptPlayback bundle that has no looping animation rewinds it to zero.
    void setIsPlaying(bool playing);
    // Pushes this player's stages onto the skeleton's queue - see the .cpp.
    void applyAnimations(Skeleton* skeleton);
    // true when not playing, or when a one-shot bundle has entered its fade-out tail.
    bool isFadingOut() const;
    // the clock - events, time, wrap, fade. Returns false when the player has finished and the
    // system should drop it. `object` receives the bundle's sound/effect events; null skips them.
    bool update(hfe::world::IObject* object, float dt);
    bool applyAnimationOnSkeleton(Skeleton* skeleton, BoneAnimation* animation,
                                  float start, float weight);
    bool applyAnimationsOnSkeleton(Skeleton* skeleton, BoneAnimation* animationA,
                                   BoneAnimation* animationB, float blend, float offset);

    // Public for the same reason Bundle's and Trigger's fields are: AnimationSystem::update /
    // playBundle / syncMovementAnimations / getPlayingAnimationLength and MovementTrigger::
    // applyAnimations all read and write these offsets from other translation units with no
    // accessor call anywhere in the binary.
    /* +0x00 */ Bundle::AnimationList::iterator m_animIt;   // the animation whose window we are in
    /* +0x04 */ Bundle::EventList::iterator     m_eventIt;  // the next event still to fire
    /* +0x08 */ Bundle*  m_bundle;
    /* +0x0C */ Trigger* m_trigger;
    /* +0x10 */ float    m_time;                 // seconds into the bundle
    /* +0x14 */ float    m_forceTime;            // AnimationSystem::update copies its own force
    /* +0x18 */ bool     m_forceTimeSet;         // time here when setForceTime has been called
    /* +0x1C */ float    m_fade;                 // the linear fade, 0..1
    /* +0x20 */ float    m_weight;               // 1.0 - no recovered writer besides resetTime
    /* +0x24 */ float    m_parameterWeight;      // setParameters' non-negative parameter
    /* +0x28 */ float    m_fadeWeight;           // m_fade squared - the factor that is applied
    /* +0x2C */ float    m_blendTransition;      // 0..1 ramp after a blend re-pairing
    /* +0x30 */ float    m_timeOffset;           // abruptPlayback's jump distance
    /* +0x34 */ bool     m_isPlaying;
    /* +0x35 */ bool     m_hasParameter;         // [CHOSEN] name - set with m_parameterWeight
    /* +0x36 */ bool     m_isMovement;           // set by the movement triggers for a looping bundle
    /* +0x37 */ bool     m_isPostTrigger;        // playBundle copies the system's post flag here
    /* +0x38 */ float    m_speed;                // signed - negative plays backward
    /* +0x3C */ int      m_prio;                 // not ctor-written; playBundle stores it
    /* +0x40 */ float    m_blendFactor;          // setBlendAnimations' blend
    /* +0x44 */ int      m_blendIndexA;          // -1 = play the bundle sequentially
    /* +0x48 */ int      m_blendIndexB;          // not ctor-written; read only when A >= 0
};

} // namespace anim
} // namespace dice

#endif // ANIMATION_BONEANIMATION_BUNDLEPLAYER_H
