
// Playback state for a bundle and its trigger.


#include "BundlePlayer.h"

#include "Bundle.h"
#include "Trigger.h"
#include "Skeleton.h"
#include "BoneAnimation.h"
#include "../../Game/Objects/IObject.h"         // IObject::handleMessage - the bundle's events
#include "../../Game/Objects/IObjectComponent.h" // TemplateMessage
#include "../../Game/Utility/Vars.h"            // Vars::getFloat - the cvar handle pattern

namespace dice {
namespace anim {

namespace {
// Backward-cycle offset as a fraction of the first animation's length.
float* const anim_offsetBackwardTime = hfe::Vars::getFloat("anim_offsetBackwardTime", 0.5f);
}
// Initializes playback and resets time; priority and the second blend index remain untouched.

BundlePlayer::BundlePlayer(Bundle& bundle, Trigger& trigger)
    : m_bundle(&bundle)
    , m_trigger(&trigger)
    , m_forceTime(0.0f)
    , m_forceTimeSet(false)
{
    resetTime();
}

// genuinely empty.
BundlePlayer::~BundlePlayer()
{
}


// Resets playback time. Fade begins at zero only when fade-in is configured.
// Preserves m_blendIndexB, m_prio, and m_forceTime.

void BundlePlayer::resetTime()
{
    m_time = 0.0f;
    Bundle* const bundle = m_bundle;
    m_timeOffset = 0.0f;
    m_fade = 0.0f;
    m_hasParameter = false;
    m_isPlaying = false;
    m_isPostTrigger = false;
    m_isMovement = false;
    m_blendIndexA = -1;
    m_blendFactor = 1.0f;
    m_weight = 1.0f;
    m_parameterWeight = 1.0f;
    m_blendTransition = 1.0f;
    m_speed = 1.0f;
    m_fadeWeight = (bundle->fadeInTime > 0.0f) ? 0.0f : 1.0f;
    m_animIt = bundle->m_animations.begin();
    m_eventIt = m_bundle->m_events.begin();
}


// Sets a blend pair for a bundle with multiple animations.
// Changing a dominant endpoint may restart the fade-in transition.

void BundlePlayer::setBlendAnimations(float blend, int indexA, int indexB)
{
    Bundle* const bundle = m_bundle;
    int count = 0;
    for (Bundle::AnimationList::const_iterator it = bundle->m_animations.begin();
         it != bundle->m_animations.end(); ++it)
        ++count;
    if (count <= 1)
        return;

    m_blendFactor = blend;
    const int previous = m_blendIndexA;
    if (previous != -1
        && ((blend > 0.7f && m_blendIndexB != indexB) || (blend < 0.3f && previous != indexA))
        && bundle->fadeInTime > 0.0f)
        m_blendTransition = 0.0f;
    m_blendIndexB = indexB;
    m_blendIndexA = indexA;
}


// Updates playback parameter and speed. A negative speed starts an inactive one-shot bundle at its end.
// Jump-to-last-at-stop forces speed 1; negative parameters are ignored.

void BundlePlayer::setParameters(float parameter, float speed)
{
    Bundle* const bundle = m_bundle;
    if (!(parameter < 0.0f) && bundle->isLooping)
    {
        m_hasParameter = true;
        m_parameterWeight = parameter;
    }

    if (bundle->jumpToLastAnimationAtStop)
        m_speed = 1.0f;
    else
        m_speed = speed;

    if (!bundle->isLooping && speed < 0.0f && !m_isPlaying)
        m_time = bundle->length;
}

// The three-factor product, in the ELF's multiplication order.
float BundlePlayer::getSlerpFactor() const
{
    return m_parameterWeight * m_weight * m_fadeWeight;
}


// Restarting an abrupt, non-looping bundle rewinds it; otherwise only stores the flag.

void BundlePlayer::setIsPlaying(bool playing)
{
    if (playing && m_isPlaying)
    {
        const Bundle* const bundle = m_bundle;
        if (bundle->abruptPlayback && !bundle->m_hasLoopingAnimation)
            m_time = 0.0f;
    }
    m_isPlaying = playing;
}


// Stopped players fade out. Scrubbed/looping playback does not; one-shot playback fades after length - fadeOutTime.

bool BundlePlayer::isFadingOut() const
{
    if (!m_isPlaying)
        return true;
    const Bundle* const bundle = m_bundle;
    if (bundle->useSpeedAsTime || bundle->isLooping
        || m_time <= bundle->length - bundle->fadeOutTime)
        return false;
    return true;
}


// Advances playback by dt, dispatches sound/effect events and updates fading.
// Returns false when playback has finished.

bool BundlePlayer::update(hfe::world::IObject* object, float dt)
{
    Bundle* bundle = m_bundle;

    // 1. events
    for (Bundle::EventList::iterator it = m_eventIt; it != bundle->m_events.end(); it = m_eventIt)
    {
        if (it->first > m_time)
            break;
        if (object)
        {
            const Bundle::BundleEvent& event = it->second;
            if (event.m_type == 1)
                object->handleMessage(static_cast<hfe::world::TemplateMessage>(event.m_id + 54), 0, false);
            else if (event.m_type == 2)
                object->handleMessage(static_cast<hfe::world::TemplateMessage>(event.m_id + 44), 0, false);
            bundle = m_bundle;
        }
        ++m_eventIt;
    }

    // 2. time
    bool pastEndHandled = false;                 // the LABEL_58 entry - see below
    if (bundle->useSpeedAsTime)
    {
        float clamped;
        if (m_speed < 0.0f)
            clamped = 0.0f;
        else
        {
            clamped = 1.0f;
            if (m_speed <= 1.0f)
                clamped = m_speed;
        }
        const float scaled = clamped * bundle->length;
        m_time = scaled;
        if (0.0f <= scaled)
        {
            float t = scaled;
            if (t > bundle->length)
                t = bundle->length;
            m_time = t;
        }
        else
            m_time = 0.0f;
    }
    else
    {
        m_time = dt * m_speed + m_time;
        if (m_time > bundle->length)
        {
            if (bundle->isLooping)
            {
                m_time = m_time - bundle->length;
                m_eventIt = bundle->m_events.begin();
            }
            else if (!bundle->playForever)
            {
                m_isPlaying = false;
                m_fadeWeight = 0.0f;
                m_fade = 0.0f;
                return false;
            }
            else
                m_time = bundle->length;
            pastEndHandled = true;
        }
        else if (m_time >= 0.0f)
            pastEndHandled = true;
        else if (bundle->isLooping)
        {
            m_time = bundle->length + m_time;
            pastEndHandled = true;
        }
        else
        {
            m_time = 0.0f;
            m_isPlaying = false;
            m_fadeWeight = 0.0f;
            m_fade = 0.0f;
            return false;
        }
    }

    // 3. looping animations inside the bundle (LABEL_58)
    if (pastEndHandled && bundle->m_hasLoopingAnimation)
    {
        if (m_isPlaying)
        {
            for (Bundle::AnimationList::iterator it = m_animIt; it != bundle->m_animations.end(); ++it)
            {
                BoneAnimation* const animation = it->second;
                if (animation->looping)
                {
                    const float now = m_time;
                    const float end = it->first + animation->length;
                    if (now > end)
                        m_time = it->first + now - end;
                }
            }
        }
        else if (bundle->abruptPlayback)
        {
            // walked backwards from the last animation, stopping at the first looping one
            for (Bundle::AnimationList::reverse_iterator it = bundle->m_animations.rbegin();
                 it != bundle->m_animations.rend(); ++it)
            {
                BoneAnimation* const animation = it->second;
                if (animation->looping)
                {
                    const float end = it->first + animation->length;
                    const float now = m_time;
                    if (end > now)
                    {
                        m_time = end;
                        m_timeOffset = end - now;
                    }
                    break;
                }
            }
        }
    }

    // 4. the fade envelope (LABEL_17)
    bool fadingOut;
    if (bundle->useSpeedAsTime || bundle->isLooping)
        fadingOut = !m_isPlaying;
    else if (m_speed < 0.0f)
        fadingOut = bundle->fadeOutTime > m_time;
    else
        fadingOut = m_time > bundle->length - bundle->fadeOutTime;

    if (fadingOut)
    {
        if (m_fadeWeight > 0.0f)
        {
            const float fadeOutTime = bundle->fadeOutTime;
            if (fadeOutTime > 0.0f)
            {
                m_fade = m_fade - dt / fadeOutTime;
                if (0.0f < m_fade)
                {
                    float fade = m_fade;
                    if (fade > 1.0f)
                        fade = 1.0f;
                    m_fade = fade;
                    m_fadeWeight = fade * fade;
                    return true;
                }
            }
        }
        m_isPlaying = false;
        m_fadeWeight = 0.0f;
        m_fade = 0.0f;
        return false;
    }

    // fading in
    if (bundle->fadeInTime > 0.0f)
    {
        if (m_blendTransition < 1.0f)
        {
            float transition = m_blendTransition + dt / bundle->fadeInTime;
            if (0.0f <= transition)
            {
                if (transition > 1.0f)
                    transition = 1.0f;
            }
            else
                transition = 0.0f;
            m_blendTransition = transition;
        }
    }

    if (bundle->fadeInTime <= 0.0f || 1.0f <= m_fadeWeight)
    {
        m_fade = 1.0f;
        m_fadeWeight = 1.0f;
        return true;
    }

    m_fade = dt / bundle->fadeInTime + m_fade;
    if (0.0f > m_fade)
    {
        m_fade = 0.0f;
        m_fadeWeight = 0.0f;
        return true;
    }

    float fade = m_fade;
    if (fade > 1.0f)
        fade = 1.0f;
    m_fade = fade;
    m_fadeWeight = fade * fade;
    return true;
}


// Stages either sequential animations or a blended animation pair on the skeleton.

void BundlePlayer::applyAnimations(Skeleton* skeleton)
{
    if (getSlerpFactor() <= 0.0f)
        return;

    const int indexA = m_blendIndexA;
    if (indexA < 0)
    {
        Bundle* bundle = m_bundle;
        Bundle::AnimationList::iterator it = m_animIt;
        if (it == bundle->m_animations.end())
            return;

        for (; it != bundle->m_animations.end(); ++it)
        {
            float start = it->first;
            if (start > m_time)
                return;

            BoneAnimation* const animation = it->second;
            const float now = m_time;
            const float end = start + animation->length;

            bool apply;
            if (now < end || bundle->playForever)
                apply = true;
            else if (now <= end || animation->looping)
            {
                start = start + m_timeOffset;
                apply = true;
            }
            else
                apply = false;

            if (apply)
            {
                Bundle::AnimationList::iterator next = it;
                ++next;
                if (next == bundle->m_animations.end()
                    || m_time <= next->first + next->second->m_blendInTime)
                {
                    float weight;
                    const float blendIn = animation->m_blendInTime;
                    const float elapsed = m_time - start;
                    if (blendIn <= 0.0f || blendIn <= elapsed)
                        weight = 1.0f;
                    else
                        weight = elapsed / blendIn;
                    applyAnimationOnSkeleton(skeleton, animation, start, weight);
                    bundle = m_bundle;
                }
            }
        }
        return;
    }

    Bundle* const bundle = m_bundle;
    int count = 0;
    for (Bundle::AnimationList::const_iterator it = bundle->m_animations.begin();
         it != bundle->m_animations.end(); ++it)
        ++count;
    if (indexA < count && m_blendIndexB >= 0)
    {
        int count2 = 0;
        for (Bundle::AnimationList::const_iterator it = bundle->m_animations.begin();
             it != bundle->m_animations.end(); ++it)
            ++count2;
        if (m_blendIndexB < count2)
        {
            BoneAnimation* const animationA = bundle->getAnimation(indexA);
            BoneAnimation* const animationB = m_bundle->getAnimation(m_blendIndexB);
            float offset = 0.0f;
            if (!m_bundle->useSpeedAsTime && m_blendIndexB == 1)
                offset = *anim_offsetBackwardTime * animationA->length;
            applyAnimationsOnSkeleton(skeleton, animationA, animationB, m_blendFactor, offset);
        }
    }
}


// Queues one animation with unit blend factor. Time is relative to start and mirrored for backward playback.
// A force-time fraction overrides that time.

bool BundlePlayer::applyAnimationOnSkeleton(Skeleton* skeleton, BoneAnimation* animation,
                                            float start, float weight)
{
    const float factor = getSlerpFactor() * weight;
    if (factor > 0.0f)
    {
        const Bundle* const bundle = m_bundle;
        float time = m_time - start;
        if (bundle->playBackward)
            time = bundle->length - time;
        if (m_forceTimeSet)
            time = m_forceTime * animation->length;
        skeleton->applyAnimation(animation, time, factor, 1.0f);
    }
    return true;
}


// Queues a blended stage only at weight >= 0.001, using m_blendTransition.
// The second sample time includes the configured offset.

bool BundlePlayer::applyAnimationsOnSkeleton(Skeleton* skeleton, BoneAnimation* animationA,
                                             BoneAnimation* animationB, float blend, float offset)
{
    const float factor = getSlerpFactor();
    if (factor > 0.001f)
    {
        float time = m_time;
        const Bundle* const bundle = m_bundle;
        if (bundle->playBackward)
            time = bundle->length - time;
        skeleton->applyAnimations(animationA, animationB, time, offset + time, blend, factor,
                                  m_blendTransition);
    }
    return true;
}

} // namespace anim
} // namespace dice
