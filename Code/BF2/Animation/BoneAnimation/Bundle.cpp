// Animation bundle ownership, events, and script serialization.

#include "Bundle.h"

#include "AnimationManager.h"         // the choke point,
#include "AnimationSystem.h"          // the owner, for TriggerManager access in deleteBundle
#include "BoneAnimation.h"            // likewise
#include "TriggerManager.h"

#include "../../Core/StringUtil.h"    // hfe::setLowerCase - the BundleEvent parser

#include <cstdio>                     // sscanf - likewise
#include <sstream>
#include <cstring>
#include "../../IO/Stream/IStream.h"

namespace dice {
namespace anim {


// Parses effect: or sound: events case-insensitively. Effect takes precedence.
// Unrecognized names retain type and ID zero; numeric suffix parsing ignores trailing text.

Bundle::BundleEvent::BundleEvent(const std::string& name)
    : m_name(name)
    , m_id(0)
    , m_type(0)
{
    hfe::setLowerCase(m_name);

    int id = 0;
    if (m_name.find("effect:", 0, 7) != std::string::npos)
    {
        sscanf(m_name.c_str(), "effect:%d", &id);
        m_type = 2;
    }
    else if (m_name.find("sound:", 0, 6) != std::string::npos)
    {
        sscanf(m_name.c_str(), "sound:%d", &id);
        m_type = 1;
    }
    m_id = id;
}


// Initializes a looping bundle with the supplied manager.

Bundle::Bundle(BundleManager* manager)
    : m_manager(manager)
    , length(1.0f)                        // +0x0C 3F800000h
    , fadeInTime(0.2f)                    // +0x10 3E4CCCCDh
    , fadeOutTime(0.1f)                   // +0x14 3DCCCCCDh
    , jumpToLastAnimationAtStop(false)
    , useSpeedAsTime(false)
    , abruptPlayback(false)
    , isLooping(true)                     // the odd one out
    , playBackward(false)
    , playForever(false)
    , m_hasLoopingAnimation(false)
{
}


// Releases shared animations through AnimationManager and deletes owned events.

Bundle::~Bundle()
{
    for (AnimationList::iterator it = m_animations.begin(); it != m_animations.end(); ++it)
        animationManager->release(it->second);
}


// Marks the bundle when it contains a looping animation and recomputes length.
// Removing the last looping animation does not clear that mark.

void Bundle::init()
{
    for (AnimationList::iterator it = m_animations.begin(); it != m_animations.end(); ++it)
    {
        if (it->second->looping)
        {
            m_hasLoopingAnimation = true;   // never cleared - see above
            break;
        }
    }
    adjustLength();
}


// Recomputes length from loaded animation endpoints and event times.
// With no animations, event times do not change the current length.

void Bundle::adjustLength()
{
    if (m_animations.empty())
        return;

    float longest = 0.0f;
    for (AnimationList::iterator it = m_animations.begin(); it != m_animations.end(); ++it)
    {
        if (it->second->m_loaded)
        {
            const float end = it->first + it->second->length;
            if (end > longest)
                longest = end;
        }
    }

    for (EventList::iterator it = m_events.begin(); it != m_events.end(); ++it)
    {
        if (it->first > longest)
            longest = it->first;
    }

    length = longest;
}

const std::string& Bundle::getName() const
{
    return m_manager->getBundleName(this);
}

// Positional access; out of range is a null, not a throw.
BoneAnimation* Bundle::getAnimation(int index) const
{
    int i = 0;
    for (AnimationList::const_iterator it = m_animations.begin(); it != m_animations.end(); ++it, ++i)
    {
        if (i == index)
            return it->second;
    }
    return 0;
}


// Finds a named animation in this bundle through the shared animation manager.
// Returns end() for an unknown animation or one absent from this bundle.

Bundle::AnimationList::iterator Bundle::getAnimation(const std::string& name)
{
    BoneAnimation* animation = animationManager->getAnimation(name);
    if (!animation)
        return m_animations.end();

    for (AnimationList::iterator it = m_animations.begin(); it != m_animations.end(); ++it)
    {
        if (it->second == animation)
            return it;
    }
    return m_animations.end();
}

// inserted at the front with start time zero - and since the list is meant to be
// sorted by start time, a fresh animation is correctly placed only because zero is the minimum.
// A name the manager cannot resolve is a silent no-op.
void Bundle::addAnimation(const std::string& name)
{
    BoneAnimation* animation = animationManager->getAnimation(name);
    if (!animation)
        return;

    m_animations.insert(m_animations.begin(), std::make_pair(0.0f, animation));
    init();
}

void Bundle::removeAnimation(const std::string& name)
{
    AnimationList::iterator it = getAnimation(name);
    if (it == m_animations.end())
        return;

    m_animations.erase(it);
    init();
}


// Changes an animation's start time and reinserts it in sorted order. An unchanged value does nothing.

void Bundle::setAnimationStartTime(const std::string& name, float startTime)
{
    AnimationList::iterator it = getAnimation(name);
    if (it == m_animations.end())
        return;
    if (it->first == startTime)
        return;                            // see above

    BoneAnimation* animation = it->second;
    m_animations.erase(it);

    AnimationList::iterator at = m_animations.begin();
    while (at != m_animations.end() && at->first < startTime)
        ++at;

    m_animations.insert(at, std::make_pair(startTime, animation));
    init();
}

// -1.0 for "NOT FOUND", not 0.0 - a real start time of zero is common, so the
// sentinel has to be outside the valid range.
float Bundle::getAnimationStartTime(const std::string& name)
{
    AnimationList::iterator it = getAnimation(name);
    return it == m_animations.end() ? -1.0f : it->first;
}


// Changes the shared BoneAnimation duration, affecting every bundle that uses that animation.

void Bundle::setAnimationLength(const std::string& name, float length_)
{
    AnimationList::iterator it = getAnimation(name);
    if (it == m_animations.end())
        return;

    it->second->setLength(length_);   // shared - see above
    init();
}

// Same -1.0 sentinel, and it reads the shared animation's length.
float Bundle::getAnimationLength(const std::string& name)
{
    AnimationList::iterator it = getAnimation(name);
    return it == m_animations.end() ? -1.0f : it->second->length;
}

// Front-inserted at time zero, like addAnimation - and with no duplicate check,
// so `addEvent sound:0_first` twice puts two identical events in the list.
void Bundle::addEvent(const std::string& name)
{
    m_events.insert(m_events.begin(), std::make_pair(0.0f, BundleEvent(name)));
    init();
}


// Finds an event by its lowercased name.

Bundle::EventList::iterator Bundle::findEvent(const std::string& name)
{
    std::string lowered(name);
    hfe::setLowerCase(lowered);

    for (EventList::iterator it = m_events.begin(); it != m_events.end(); ++it)
    {
        if (it->second.m_name == lowered)
            return it;
    }
    return m_events.end();
}

// . Erases the first match only - and addEvent allows
// duplicates, so removing a doubly-added event takes two calls.
void Bundle::removeEvent(const std::string& name)
{
    EventList::iterator it = findEvent(name);
    if (it == m_events.end())
        return;

    m_events.erase(it);
    init();
}


// Moves an event to its new time while preserving sorted order. An unchanged time is a no-op.

void Bundle::setEventStartTime(const std::string& name, float startTime)
{
    EventList::iterator it = findEvent(name);
    if (it == m_events.end())
        return;
    if (it->first == startTime)
        return;                            // see above

    const BundleEvent event = it->second;  // copied before the erase
    m_events.erase(it);

    EventList::iterator at = m_events.begin();
    while (at != m_events.end() && at->first < startTime)
        ++at;

    m_events.insert(at, std::make_pair(startTime, event));
    init();
}

// -1.0 for "NOT FOUND", the same sentinel its two animation twins use - and for
// the same reason: an event at time zero is the common case, so zero cannot mean absent.
float Bundle::getEventStartTime(const std::string& name)
{
    EventList::iterator it = findEvent(name);
    return it == m_events.end() ? -1.0f : it->first;
}


// Creates and selects a named bundle. Duplicate names return null and clear the active selection.

Bundle* BundleManager::createBundle(const std::string& name)
{
    if (m_bundles.find(name) != m_bundles.end())
    {
        m_activeBundle = 0;               // see above
        return 0;
    }

    Bundle* bundle = new Bundle(this);    // operator new(0x20)
    m_activeBundle = bundle;
    m_bundles.insert(std::make_pair(name, bundle));
    return bundle;
}


// Removes trigger references before destroying a bundle. An unregistered bundle is still destroyed.

void BundleManager::deleteBundle(Bundle* bundle)
{
    if (!bundle)
        return;

    m_owner->getTriggerManager()->removeBundleFromAllTriggers(bundle);
    m_bundles.erase(getBundleName(bundle));
    delete bundle;
}

// The null-out happens after the delete and unconditionally.
void BundleManager::deleteActiveBundle()
{
    deleteBundle(m_activeBundle);
    m_activeBundle = 0;
}

// A miss clears the cursor rather than leaving it alone - the same rule
// setActiveSystem and setActiveByName follow.
void BundleManager::setActiveBundle(const std::string& name)
{
    std::map<std::string, Bundle*, hfe::NoCaseStringCompare>::iterator it = m_bundles.find(name);
    m_activeBundle = (it == m_bundles.end()) ? 0 : it->second;
}

Bundle* BundleManager::getActiveBundlePtr() const { return m_activeBundle; }

// The round trip: hand the active pointer back to the name lookup.
const std::string& BundleManager::getActiveBundle() const
{
    return getBundleName(m_activeBundle);
}

// A keyed find; a miss is a null.
Bundle* BundleManager::getBundleByName(const std::string& name)
{
    std::map<std::string, Bundle*, hfe::NoCaseStringCompare>::iterator it = m_bundles.find(name);
    return it == m_bundles.end() ? 0 : it->second;
}


// Returns bundle's registered name or a stable no-bundle-found string.

const std::string& BundleManager::getBundleName(const Bundle* bundle) const
{
    static const std::string s_notFound("no bundle found");

    if (!bundle)
        return s_notFound;

    for (std::map<std::string, Bundle*, hfe::NoCaseStringCompare>::const_iterator it =
             m_bundles.begin(); it != m_bundles.end(); ++it)
    {
        if (it->second == bundle)
            return it->first;
    }
    return s_notFound;
}


BundleManager::BundleManager(AnimationSystemTemplate* owner)
    : m_activeBundle(0)
    , m_owner(owner)
{
}

// Deletes every bundle without notifying triggers; the owning template also destroys the trigger manager.
BundleManager::~BundleManager()
{
    for (std::map<std::string, Bundle*, hfe::NoCaseStringCompare>::iterator it = m_bundles.begin();
         it != m_bundles.end(); ++it)
    {
        if (it->second)
            delete it->second;
    }
    m_bundles.clear();
}


// Initializes all bundles after animation files load so duration calculations use final lengths.
// Requires nonnull bundle entries.

void BundleManager::postLoad()
{
    for (std::map<std::string, Bundle*, hfe::NoCaseStringCompare>::iterator it = m_bundles.begin();
         it != m_bundles.end(); ++it)
    {
        it->second->init();               // unguarded - see above
    }
}

// Saves animations in reverse order and events forward; emits only positive start times and omits length.
void Bundle::makeScript(hfe::io::IStream* stream)
{
    std::ostringstream output;
    output << "animationSystem.createBundle " << getName() << std::endl;
    if (!isLooping) output << "animationBundle.isLooping 0" << std::endl;
    if (abruptPlayback) output << "animationBundle.abruptPlayback 1" << std::endl;
    if (playBackward) output << "animationBundle.playBackward 1" << std::endl;
    if (fadeInTime != 0.2f) output << "animationBundle.fadeInTime " << fadeInTime << std::endl;
    if (fadeOutTime != 0.1f) output << "animationBundle.fadeOutTime " << fadeOutTime << std::endl;
    if (playForever) output << "animationBundle.playForever 1" << std::endl;
    if (useSpeedAsTime) output << "animationBundle.useSpeedAsTime 1" << std::endl;
    if (jumpToLastAnimationAtStop)
        output << "animationBundle.jumpToLastAnimationAtStop 1" << std::endl;
    for (AnimationList::reverse_iterator it = m_animations.rbegin(); it != m_animations.rend(); ++it)
    {
        const std::string& name = animationManager->getName(it->second);
        output << "animationBundle.addAnimation " << name << std::endl;
        if (it->first > 0.0f)
            output << "animationBundle.setAnimationStartTime " << name << " " << it->first << std::endl;
    }
    for (EventList::iterator it = m_events.begin(); it != m_events.end(); ++it)
    {
        output << "animationBundle.addEvent " << it->second.m_name << std::endl;
        if (it->first > 0.0f)
            output << "animationBundle.setEventStartTime " << it->second.m_name << " " << it->first << std::endl;
    }
    output << std::endl;
    const std::string text = output.str();
    stream->write(text.c_str(), static_cast<unsigned int>(text.size()));
}

// Returns listings as strings without printing them.
std::string Bundle::listAnimations() const
{
    std::stringstream output;
    for (AnimationList::const_iterator it = m_animations.begin(); it != m_animations.end(); ++it)
        output << animationManager->getName(it->second) << "\n";
    return output.str();
}

std::string Bundle::listEvents() const
{
    std::stringstream output;
    for (EventList::const_iterator it = m_events.begin(); it != m_events.end(); ++it)
        output << it->second.m_name << "\n";
    return output.str();
}

// . insert preserves an existing equivalent key; no clear/overwrite.
void Bundle::getAnimations(std::map<std::string, BoneAnimation*, hfe::NoCaseStringCompare>& out) const
{
    for (AnimationList::const_iterator it = m_animations.begin(); it != m_animations.end(); ++it)
        out.insert(std::make_pair(animationManager->getName(it->second), it->second));
}

// . Shared animations appear once, in case-insensitive name order,
// before the bundles. All write return values are discarded, including short writes.
void BundleManager::makeScript(hfe::io::IStream* stream)
{
    const char* header = "\nrem *** Animations ***\n";
    stream->write(header, static_cast<unsigned int>(std::strlen(header)));
    std::map<std::string, BoneAnimation*, hfe::NoCaseStringCompare> animations;
    for (auto it = m_bundles.begin(); it != m_bundles.end(); ++it)
        it->second->getAnimations(animations);
    for (auto it = animations.begin(); it != animations.end(); ++it)
        it->second->makeScript(stream);
    header = "\nrem *** Bundles ***\n";
    stream->write(header, static_cast<unsigned int>(std::strlen(header)));
    for (auto it = m_bundles.begin(); it != m_bundles.end(); ++it)
        it->second->makeScript(stream);
}

// ; constness from the ELF mangling, return from both callers.
std::string BundleManager::listBundles() const
{
    std::stringstream output;
    for (auto it = m_bundles.begin(); it != m_bundles.end(); ++it)
        output << it->first << "\n";
    return output.str();
}

} // namespace anim
} // namespace dice
