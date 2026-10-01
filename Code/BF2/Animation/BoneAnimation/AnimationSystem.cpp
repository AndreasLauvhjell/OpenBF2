
#include "AnimationSystem.h"

#include <cmath>                          // fabsf - getPlayingAnimationLength's backward arm
#include <cstdio>                         // [TEMP] fprintf/fflush - the short-circuit tracer
#include <cstring>                        // strlen - create()
#include <sstream>

#include "../../Core/Debug.h"
#include "../../Core/EventManager.h"      // eventManager - the deferred-load registration
#include "../../Core/LoadStat.h"          // hfe::NextLoadPart - the progress payload
#include "../../IO/File/File.h"           // create() writes a stub .inc
#include "../../IO/Console/Console.h"     // io::mainConsole - loadScript runs the .inc
#include "../../IO/Stream/IStream.h"      // ... through the raw stream, not through File::write

#include "Bundle.h"                       // BundleManager,
#include "CameraSpring.h"                 // CameraSpring,
#include "Trigger.h"
#include "BundlePlayer.h"                 // the players update() drives
#include "Skeleton.h"                     // applyOnSkeleton writes the skeleton's +0x00
#include "../../Game/Utility/Vars.h"      // Vars::getInt/getFloat - the two cvar handles
#include <cfloat>                         // FLT_EPSILON - setTriggerMovement's normalise
#include "TriggerManager.h"               // TriggerManager,
#include "../../Core/System.h"            // g_system - disable()'s timestamp
#include "AnimationManager.h"
#include "ValueHolder.h"                  // ValueHolderManager - loadScript primes it

namespace dice {
namespace anim {

AnimationSystemTemplateManager* animationSystemTemplateManager = 0;
// Global animation time scale.
float AnimationSystemTemplate::globalTimeModifier = 1.0f;


namespace {

} // namespace


// Initializes template ownership and reference state. The unidentified trailing word remains uninitialized.

AnimationSystemTemplate::AnimationSystemTemplate()
    : m_triggerManager(0)
    , m_bundleManager(0)
    , m_cameraSpring(0)
    , m_refCount(0)                       // zero, not one - load() addRefs before handing it back
{
    // m_dword14 deliberately left alone; see above.
}


// Releases template managers and frees the camera spring without calling its destructor.

AnimationSystemTemplate::~AnimationSystemTemplate()
{
    delete m_triggerManager;                  // guarded in the ELF; `delete` covers the null itself
    delete m_bundleManager;
    // CameraSpring currently owns no resources requiring destruction.
    operator delete(m_cameraSpring);
}

void AnimationSystemTemplate::addRef()
{
    ++m_refCount;
}


// Drops a reference and deletes the template through its virtual destructor when the last reference is released.

int AnimationSystemTemplate::release()
{
    const bool wasLast = (m_refCount == 1);
    const int remaining = --m_refCount;
    if (wasLast)
        delete this;                          // vtable +0x04, the D0 deleting destructor
    return remaining;
}

// AnimationSystem is 132 bytes and takes its template in the constructor.
AnimationSystem* AnimationSystemTemplate::makeInstance()
{
    return new AnimationSystem(*this);     // operator new(0x84)
}


// Returns the manager's registration key for this template.

const std::string& AnimationSystemTemplate::getPath() const
{
    return animationSystemTemplateManager->getPath(this);
}

CameraSpring* AnimationSystemTemplate::getCameraSpring() const { return m_cameraSpring; }
bool AnimationSystemTemplate::getUseCameraSpring() const { return m_cameraSpring != 0; }

// idempotent in both directions: setting it twice does not build a second spring,
// clearing it twice does not double-free. And the clear here has the same missing ~CameraSpring as
// the destructor - the two sites agree with each other, which is why this reads as a habit rather
// than a one-off typo.
void AnimationSystemTemplate::setUseCameraSpring(bool use)
{
    if (use)
    {
        if (!m_cameraSpring)
            m_cameraSpring = new CameraSpring();   // operator new(0x50) + the ctor
    }
    else if (m_cameraSpring)
    {
        operator delete(m_cameraSpring);
        m_cameraSpring = 0;
    }
}

// getPath through the vtable (+0x18), then save to whatever came back.
void AnimationSystemTemplate::saveActive()
{
    save(getPath());
}


// Loads the animation script once, with this template selected for its console commands.
// Returns false if already initialized or if script execution fails. Trigger priorities are assigned only on success.
// Restores the caller's relative-path setting after execution.

bool AnimationSystemTemplate::loadScript(const std::string& path)
{
    if (m_bundleManager)
        return false;                         // already loaded - see above

    // A process-wide one-shot: every animation system of every soldier calls this, and exactly
    // one of them does any work.
    valueHolderManager->loadValueHolders();

    m_bundleManager  = new BundleManager(this);      // operator new(0x14)
    m_triggerManager = new TriggerManager(this);     // operator new(0x14)

    const bool wasRelative = hfe::io::mainConsole->getUseRelativePaths();
    hfe::io::mainConsole->setUseRelativePaths(false);

    // asked before the RUN - see above.
    const bool wasUnregistered = getPath().empty();

    animationSystemTemplateManager->setActive(this);   // the binding target

    const std::string empty;
    const bool ok = hfe::io::mainConsole->run(path, empty, empty, empty, empty, empty, empty, empty,
                                              empty, hfe::io::Console::ignoredString_);

    if (wasUnregistered)
        animationSystemTemplateManager->hide();       // reaches the AnimationManager ghost

    if (ok)
        m_triggerManager->setPriosForTriggers();

    hfe::io::mainConsole->setUseRelativePaths(wasRelative);
    return ok;
}


// Writes camera spring, bundles and triggers in dependency order. Requires initialized bundle and trigger managers.

void AnimationSystemTemplate::save(const std::string& path)
{
    hfe::io::File file;
    file.open(path, static_cast<hfe::io::IFileSystem::FileMode>(0),
              hfe::io::IFileSystem::FILEACCESS_2, 0);
    hfe::io::IStream* stream = file.getStream();

    if (m_cameraSpring)
        m_cameraSpring->makeScript(stream);
    m_bundleManager->makeScript(stream);                  // unguarded
    m_triggerManager->makeScript(stream);                 // unguarded

    file.close();
}


// Clears trigger and bundle state before attempting a reload.
// The replacement bundle manager causes loadScript() to return false, leaving the template empty.

bool AnimationSystemTemplate::reloadScript()
{
    delete m_triggerManager;                  // guarded in the ELF; `delete` covers the null itself
    delete m_bundleManager;

    m_bundleManager  = new BundleManager(this);
    m_triggerManager = new TriggerManager(this);

    const std::string& path = getPath();      // vtable +0x18
    if (path.empty())
        return false;

    return loadScript(path);                  // always false - see above
}


// Enables deferred loading and registers for the event that completes it.

AnimationSystemTemplateManager::AnimationSystemTemplateManager()
    : m_deferLoading(true)                    // byte +0x08, the defer switch, on from the start
    , m_active(0)                             // +0x18
    , m_playbackModifier(1.0f)                // +0x1C 3F800000h
{
    if (m_deferLoading
        && !hfe::eventManager->registerEventHandler(static_cast<hfe::EventCategory>(5), this, 0))
    {
        hfe::Debug(hfe::DEBUGTYPE_6, "Animation/BoneAnimation/AnimationSystem.cpp", 85, "")
            << "Failed to register AnimationSystemTemplateManager as event handler for ECCore.";
    }
}


// Deletes all registered templates regardless of their reference counts.

AnimationSystemTemplateManager::~AnimationSystemTemplateManager()
{
    if (m_deferLoading)
        hfe::eventManager->unregisterEventHandler(static_cast<hfe::EventCategory>(5), this);

    for (std::map<std::string, AnimationSystemTemplate*, hfe::NoCaseStringCompare>::iterator it =
             m_templates.begin(); it != m_templates.end(); ++it)
    {
        if (it->second)
            delete it->second;                // vtable +0x04, deleting destructor - not release()
    }
    m_templates.clear();
}


// Writes a new animation script containing the completeTree root trigger.

void AnimationSystemTemplateManager::create(const std::string& path)
{
    static const char* const kStubLine = "animationSystem.createTrigger Trigger completeTree";

    hfe::io::File file;
    file.open(path, static_cast<hfe::io::IFileSystem::FileMode>(0),
              hfe::io::IFileSystem::FILEACCESS_2, 0);

    hfe::io::IStream* stream = file.getStream();
    stream->write(kStubLine, static_cast<unsigned int>(strlen(kStubLine)));

    file.close();
}


// Returns a referenced cached template or creates one. Loading is deferred when configured.
// Immediate load failure destroys the new template and returns null.

AnimationSystemTemplate* AnimationSystemTemplateManager::load(const std::string& name)
{
    std::map<std::string, AnimationSystemTemplate*, hfe::NoCaseStringCompare>::iterator it =
        m_templates.find(name);
    if (it != m_templates.end())
    {
        it->second->addRef();
        return it->second;
    }

    AnimationSystemTemplate* templ = new AnimationSystemTemplate();   // operator new(0x18)

    // the defer: with the switch on, the right-hand side never runs and the file is not touched.
    if (m_deferLoading || templ->loadScript(name))
    {
        m_templates.insert(std::make_pair(name, templ));
        templ->addRef();
        return templ;
    }

    delete templ;                             // vtable +0x04 - see above
    return 0;
}


// Releases a template reference and removes the corresponding registry entry when it is destroyed.

int AnimationSystemTemplateManager::release(IAnimationSystemTemplate* templ)
{
    if (!templ)
        return 0;

    const int remaining = templ->release();   // vtable +0x0C - and it may delete `templ` outright
    if (remaining == 0)
    {
        for (std::map<std::string, AnimationSystemTemplate*, hfe::NoCaseStringCompare>::iterator it =
                 m_templates.begin(); it != m_templates.end(); ++it)
        {
            if (it->second == templ)          // comparing a dangling pointer by value; dice's
            {
                m_templates.erase(it);
                break;
            }
        }
    }

    setActive(0);                             // unconditional - see above
    return remaining;
}

// Look the name up and make it active.
// A miss clears the active template rather than leaving it alone - setActiveSystem("typo") is a
// working way to deactivate.
void AnimationSystemTemplateManager::setActiveSystem(const std::string& name)
{
    std::map<std::string, AnimationSystemTemplate*, hfe::NoCaseStringCompare>::iterator it =
        m_templates.find(name);
    setActive(it == m_templates.end() ? 0 : it->second);
}


// Finds a template's registered path by its pointer.

const std::string& AnimationSystemTemplateManager::getActiveSystem() const
{
    static const std::string s_noActiveSystem;

    if (!m_active)
        return s_noActiveSystem;

    for (std::map<std::string, AnimationSystemTemplate*, hfe::NoCaseStringCompare>::const_iterator
             it = m_templates.begin(); it != m_templates.end(); ++it)
    {
        // Preserve the active-template lookup on each iteration.
        if (it->second == m_active)
            return it->first;
    }
    return s_noActiveSystem;
}

const std::string& AnimationSystemTemplateManager::getPath(const IAnimationSystemTemplate* templ) const
{
    static const std::string s_noPath;

    if (!templ)
        return s_noPath;

    for (std::map<std::string, AnimationSystemTemplate*, hfe::NoCaseStringCompare>::const_iterator
             it = m_templates.begin(); it != m_templates.end(); ++it)
    {
        if (it->second == templ)
            return it->first;
    }
    return s_noPath;
}

void  AnimationSystemTemplateManager::setActive(AnimationSystemTemplate* t) { m_active = t; }
AnimationSystemTemplate* AnimationSystemTemplateManager::getActive() const  { return m_active; }
void  AnimationSystemTemplateManager::setPlaybackModifier(float m) { m_playbackModifier = m; }
float AnimationSystemTemplateManager::getPlaybackModifier() const { return m_playbackModifier; }


// Clears the active template and the sibling managers' active selections.

void AnimationSystemTemplateManager::hide()
{
    setActive(0);                             // vtable +0x20
    valueHolderManager->setActive(0);
    // Requires the value-holder manager.
    animationManager->setActiveAnimation(0);
}


// Completes deferred loading on core event 3: scripts, animations, then bundle post-load processing.

void AnimationSystemTemplateManager::handleEvent(hfe::EventCategory category, unsigned int id,
                                                 hfe::EventNode* /*node*/, void* /*data*/)
{
    if (category != hfe::EventCategory::ECCore || id != 3)      // ECCore, [UNRESOLVED] id-3 name
        return;

    {
        hfe::NextLoadPart part = { static_cast<int>(m_templates.size()), 0.1f, "AnimationSystems" };
        hfe::eventManager->postEvent(hfe::EventCategory::ECCore, 4, &part, 0.0f);
        hfe::eventManager->postEvent(hfe::EventCategory::ECHud, 7,
                                const_cast<char*>("Loading AnimationSystems.."), 0.0f);
    }

    // [TEMP] [CHOSEN, not recovered] Traces deferred-load short circuits and template lifetime.
    // [RETIRE-WHEN: bone-staging-index] Remove after repeated level loads preserve animation ownership and indices.
    static int s_handleEventCalls = 0;
    const int callNumber = ++s_handleEventCalls;
    int reran = 0;

    for (std::map<std::string, AnimationSystemTemplate*, hfe::NoCaseStringCompare>::iterator it =
             m_templates.begin(); it != m_templates.end(); ++it)
    {
        if (it->second->loadScript(it->first))  // the key is the path
            ++reran;                            // [TEMP]
    }

    // [TEMP] - see above.
    std::fprintf(stderr, "[TEMP][ANIMSWEEP] handleEvent #%d: %u templates registered, %d re-read "
                         "their script.\n",
                 callNumber, static_cast<unsigned>(m_templates.size()), reran);
    std::fflush(stderr);

    {
        hfe::NextLoadPart part = { animationManager->getNumAnimationsToLoad(), 0.5f, "Animations" };
        hfe::eventManager->postEvent(hfe::EventCategory::ECCore, 4, &part, 0.0f);
        hfe::eventManager->postEvent(hfe::EventCategory::ECHud, 7,
                                const_cast<char*>("Loading Animations.."), 0.0f);
    }

    animationManager->postLoad();

    for (std::map<std::string, AnimationSystemTemplate*, hfe::NoCaseStringCompare>::iterator it =
             m_templates.begin(); it != m_templates.end(); ++it)
    {
        // unguarded, and it is safe only because of where loadScript creates its managers: both
        // are `new`ed before the console run and neither is unwound on failure, so every template
        // that has been through pass one above has a bundle manager whatever the script did.
        it->second->m_bundleManager->postLoad();
    }
}


// Runtime animation playback.

namespace {
int*   const anim_skipSync  = hfe::Vars::getInt("anim_skipSync", 0);
float* const anim_smoothSet = hfe::Vars::getFloat("anim_smoothSet", 10.0f);

// The sign helper setTriggerMovement inlines four times: 1 above zero, -1 below, 0 at zero (and
// 1 for NaN, since the first test is `<= 0`). Not an ELF symbol.
float signOf(float v)
{
    float sign = 1.0f;
    if (v <= 0.0f)
    {
        sign = -1.0f;
        if (v >= 0.0f)
            sign = 0.0f;
    }
    return sign;
}
} // namespace


// Initializes playback state with full weight and references to templ.

AnimationSystem::AnimationSystem(AnimationSystemTemplate& templ)
    : m_triggerPose(0)
    , m_triggerMessages(0)
    , m_lastTriggerMessages(0)
    , m_triggerOnceMessages(0)
    , m_movementSpeed(0.0f)
    , m_idleTime(0.0f)
    , m_postIdleTime(0.0f)
    , m_movementPhase(0.0f)
    , m_template(&templ)                  // +0x48
    , m_templateAgain(&templ)             // +0x4C the same pointer, twice
    , m_activeTrigger(0)
    , m_postRootTrigger(0)
    , m_inPostTrigger(false)
    , m_startupPending(true)              // the one bool that starts true
    , m_locked(false)
    , m_playing(false)
    , m_fadeInTime(0.0f)
    , m_weight(1.0f)                      // 3F800000h - the skeleton-wide animation weight
    , m_fadeInFrom(0)
    , m_needsInit(true)                   // also true; update() calls init() and clears it
    , m_forceTimeSet(false)
    , m_reenableTime(-1.0)
{
    m_movement.x = 0.0f;     m_movement.y = 0.0f;     m_movement.z = 0.0f;       // +0x14..+0x1C
    m_movementRate.x = 0.0f; m_movementRate.y = 0.0f; m_movementRate.z = 0.0f;   // +0x24..+0x2C
    m_look.x = 0.0f;         m_look.y = 0.0f;         m_look.z = 0.0f;           // +0x30..+0x38
    // m_dword60 and m_forceTime deliberately left alone - see the header.
}

// Releases every running bundle player.
AnimationSystem::~AnimationSystem()
{
    for (std::list<BundlePlayer*>::iterator it = m_players.begin(); it != m_players.end(); ++it)
        releaseBundlePlayer(*it);
    m_players.clear();
}


// Resolves the optional postRoot trigger, retaining null when the script does not define one.

void AnimationSystem::init()
{
    m_needsInit = false;
    m_postRootTrigger = m_template->getTriggerManager()->getTriggerByName(std::string("postRoot"), false);
}

AnimationSystemTemplate* AnimationSystem::getTemplate() { return m_templateAgain; }


// Stops playback and resets runtime state, preserving the template, post-root trigger, and force time.

void AnimationSystem::reset()
{
    stopAllPlaying();

    m_startupPending      = true;
    m_weight              = 1.0f;
    m_playing             = false;
    m_activeTrigger       = 0;
    m_locked              = false;
    m_fadeInFrom          = 0;
    m_fadeInTime          = 0.0f;
    m_idleTime            = 0.0f;
    m_postIdleTime        = 0.0f;
    m_triggerOnceMessages = 0;
    m_triggerMessages     = 0;
    m_lastTriggerMessages = 0;
}


// With true, schedules a 0.3-second delay; with false, stops playback and resets state immediately.

void AnimationSystem::disable(bool disable_)
{
    if (disable_)
    {
        m_reenableTime = hfe::g_system->getExactTime() + 0.3;   // a double, 8-byte store
        return;
    }
    stopAllPlaying();
    reset();                              // through the vtable, +0x0C
}

// operator new(0x4C) - a BundlePlayer is 76 bytes.
// it does not ADD the player to m_players. The caller does; playBundle is [FRONTIER] and is where
// that happens.
BundlePlayer* AnimationSystem::createBundlePlayer(Bundle* bundle, Trigger* trigger)
{
    return new BundlePlayer(*bundle, *trigger);
}

// A member that ignores `this` entirely - it is a destroy helper, not a pool.
void AnimationSystem::releaseBundlePlayer(BundlePlayer* player)
{
    if (player)
        delete player;
}

// Release every player, then clear the list. The destructor is this plus the
// list's own teardown.
void AnimationSystem::stopAllPlaying()
{
    for (std::list<BundlePlayer*>::iterator it = m_players.begin(); it != m_players.end(); ++it)
        releaseBundlePlayer(*it);
    m_players.clear();
}


// Copies trigger's fade-in time only when the trigger changes.

void AnimationSystem::setFadeInTime(Trigger* trigger)
{
    if (m_fadeInFrom == trigger)
        return;

    m_fadeInFrom = trigger;
    m_fadeInTime = trigger->fadeInTime;
}


// Applies matching bundle players to the skeleton with this system's animation weight.

void AnimationSystem::applyOnSkeleton(Skeleton* skeleton, bool postTrigger)
{
    skeleton->m_float00 = m_weight;                  // the skeleton's +0x00

    for (std::list<BundlePlayer*>::iterator it = m_players.begin(); it != m_players.end(); ++it)
    {
        if ((*it)->m_isPostTrigger == postTrigger)
            (*it)->applyAnimations(skeleton);
    }
}

// One byte picks which of two idle timers to report - the post-trigger flag.
float AnimationSystem::getIdleTime()
{
    return m_inPostTrigger ? m_postIdleTime : m_idleTime;
}

// Sticky messages support set/clear; one-shot messages are accumulated for update.
void AnimationSystem::setTriggerMessage(unsigned int mask)     { m_triggerMessages |= mask; }
void AnimationSystem::setTriggerOnceMessage(unsigned int mask) { m_triggerOnceMessages |= mask; }
void AnimationSystem::clearTriggerMessage(unsigned int mask)   { m_triggerMessages &= ~mask; }

// The getter reads a field the constructor never writes; the flag the setter
// raises is what keeps that garbage from being trusted. Reproduced, both halves.
void  AnimationSystem::setForceTime(float t) { m_forceTimeSet = true; m_forceTime = t; }
float AnimationSystem::getForceTime()        { return m_forceTime; }


// Writes the highest-priority matching animation's length and playback phase.
// Both outputs are -1 when no animation matches. movementOnly restricts the query to locomotion.
// Backward playback reports a mirrored phase.

void AnimationSystem::getPlayingAnimationLength(float& length, float& time, bool movementOnly)
{
    length = -1.0f;
    time   = -1.0f;

    BundlePlayer* playing = 0;
    for (std::list<BundlePlayer*>::reverse_iterator it = m_players.rbegin();
         it != m_players.rend(); ++it)
    {
        if (!movementOnly || (*it)->m_isMovement)
        {
            playing = *it;
            break;
        }
    }

    if (!playing)
        return;

    const float speed = playing->m_speed;
    if (speed > 0.0f)
    {
        length = playing->m_bundle->length * speed;
        time   = speed * playing->m_time;
    }
    else
    {
        // The backward arm. |speed| scales both, and the time is taken from the other end - the
        // just-written length is reused, which is literally what both compilers emit (fst, not fstp).
        const float scale = fabsf(speed);
        length = playing->m_bundle->length * scale;
        time   = length - scale * playing->m_time;
    }
}


// Selects or plays a named trigger, subject to the current exclusive-playback lock.

void AnimationSystem::setActiveTrigger(const std::string& name, bool lock)
{
    if (m_locked)
        return;

    Trigger* trigger = m_template->getTriggerManager()->getTriggerByName(name, false);
    if (!trigger)
        return;

    stopAllPlaying();
    m_activeTrigger = trigger;
    m_locked = lock;
}

bool AnimationSystem::playTrigger(const std::string& name, bool exclusive)
{
    if (m_locked)
        return false;

    Trigger* trigger = m_template->getTriggerManager()->getTriggerByName(name, false);
    if (!trigger)
        return false;

    if (exclusive)
        stopAllPlaying();

    trigger->update(*this);               // Trigger vtable +0x4C
    m_playing = true;
    m_locked = exclusive;
    return true;
}

// it brackets the trigger'S update() with the post flag - which is what makes getIdleTime report
// the other timer for the duration, and presumably what the trigger tree branches on.
bool AnimationSystem::playPostTrigger(const std::string& name, bool exclusive)
{
    if (m_locked || m_playing)
        return false;

    Trigger* trigger = m_template->getTriggerManager()->getTriggerByName(name, false);
    if (!trigger)
        return false;

    m_inPostTrigger = true;
    trigger->update(*this);               // Trigger vtable +0x4C
    m_inPostTrigger = false;
    m_locked = exclusive;
    return true;
}


// Advances animation players and trigger state for object by dt, initializing lazily on first use.

void AnimationSystem::update(hfe::world::IObject* object, float dt)
{
    if (m_needsInit)
        init();

    if (m_reenableTime >= 0.0)
    {
        const float now = static_cast<float>(hfe::g_system->getExactTime());
        if (now > m_reenableTime)
        {
            m_reenableTime = -1.0;
            stopAllPlaying();
            reset();                          // through the vtable, +0x0C
        }
        else
            m_reenableTime = -1.0;            // as shipped - see the note above
    }

    bool messagesChanged = false;
    if (!m_locked)
    {
        Trigger* active = m_activeTrigger;
        if (!active && !m_playing)
        {
            if (m_startupPending)
            {
                m_startupPending = false;
                Trigger* const startup =
                    m_template->getTriggerManager()->getTriggerByName(std::string("startup"), true);
                if (startup)
                {
                    startup->update(*this);
                    m_playing = true;
                }
                active = m_activeTrigger;
            }
            if (!active)
            {
                m_activeTrigger =
                    m_template->getTriggerManager()->getTriggerByName(std::string("root"), true);
                active = m_activeTrigger;
            }
        }

        const unsigned int messages = m_triggerMessages | m_triggerOnceMessages;
        messagesChanged = (m_lastTriggerMessages != messages);
        m_triggerMessages = messages;
        if (active)
            active->update(*this);
        if (m_postRootTrigger)
        {
            m_inPostTrigger = true;
            m_postRootTrigger->update(*this);
            m_inPostTrigger = false;
        }
        const unsigned int once = m_triggerOnceMessages;
        m_triggerOnceMessages = 0;
        m_triggerMessages = m_triggerMessages & ~once;
        m_lastTriggerMessages = m_triggerMessages;
    }

    m_weight = 1.0f;
    if (m_playing)
    {
        m_fadeInTime = 0.0f;
    }
    else if (m_fadeInTime > 0.0f && m_fadeInFrom)
    {
        m_fadeInTime = m_fadeInTime - dt;
        if (!(m_fadeInTime > 0.0f))
            m_fadeInTime = 0.0f;
        m_weight = 1.0f - m_fadeInTime / m_fadeInFrom->fadeInTime;
    }

    int ordinaryCount = 0;
    int postCount = 0;
    for (std::list<BundlePlayer*>::iterator it = m_players.begin(); it != m_players.end(); )
    {
        std::list<BundlePlayer*>::iterator next = it;
        ++next;
        BundlePlayer* const player = *it;

        if (player->m_isPostTrigger)
            ++postCount;
        else
            ++ordinaryCount;

        if (m_playing && player->isFadingOut())
            m_playing = false;

        if (m_forceTimeSet)
        {
            player->m_forceTime = m_forceTime;
            player->m_forceTimeSet = true;
        }

        Trigger* const trigger = player->m_trigger;
        const bool alive = player->update(object, dt);

        bool drop;
        if (!alive)
            drop = true;
        else if (trigger->m_underIdle)
            drop = messagesChanged || next != m_players.end();
        else
            drop = false;

        if (drop)
        {
            if (trigger->m_underIdle)
                m_fadeInFrom = 0;
            m_players.erase(it);
            releaseBundlePlayer(player);
            m_playing = false;
        }
        else
        {
            const Bundle* const bundle = player->m_bundle;
            if (bundle->isLooping || bundle->m_hasLoopingAnimation)
                player->setIsPlaying(false);
        }
        it = next;
    }

    if (ordinaryCount == 1)
        m_idleTime = m_idleTime + dt;
    else
        m_idleTime = 0.0f;
    if (postCount == 1)
        m_postIdleTime = m_postIdleTime + dt;
    else
        m_postIdleTime = 0.0f;
}


// Finds or creates a bundle player in ascending priority order. Returns the player.
// Priorities above 9000 stop existing playback; changing an existing player's priority leaves its list position intact.

BundlePlayer* AnimationSystem::playBundle(Trigger* trigger, Bundle* bundle, int prio,
                                          float parameter, float speed)
{
    if (prio > 9000)
        stopAllPlaying();

    BundlePlayer* player = 0;
    std::list<BundlePlayer*>::iterator insertAt = m_players.end();
    for (std::list<BundlePlayer*>::iterator it = m_players.begin(); it != m_players.end(); ++it)
    {
        BundlePlayer* const candidate = *it;
        if (bundle == candidate->m_bundle)
        {
            player = candidate;
            break;
        }
        if (candidate->m_prio > prio)
        {
            insertAt = it;
            break;
        }
    }

    if (!player)
    {
        player = createBundlePlayer(bundle, trigger);
        m_players.insert(insertAt, player);
    }

    player->setParameters(parameter, speed);
    player->setIsPlaying(true);
    player->m_prio = prio;                                    // +0x3C
    player->m_isPostTrigger = m_inPostTrigger;                // +0x37 <- +0x58
    return player;
}


// Reads message masks from other, then copies this system's movement and optionally pose into other.

void AnimationSystem::copyStateToSystem(IAnimationSystem* other, bool copyPose)
{
    const AnimationSystem* const source = static_cast<const AnimationSystem*>(other);
    m_triggerMessages     = source->m_triggerMessages;
    m_triggerOnceMessages = source->m_triggerOnceMessages;
    if (copyPose)
        other->setTriggerPose(m_triggerPose);
    other->setTriggerMovement(-1.0f, m_movement, m_movementSpeed, m_look);
}


// Selects the pose-transition trigger using the current movement direction and speed.

void AnimationSystem::setTriggerPose(unsigned int pose)
{
    const unsigned int current = m_triggerPose;
    if (pose == current)
        return;

    Trigger* trigger = 0;
    if (pose == 2)
    {
        const char* name;
        if (m_movementSpeed <= 2.0f)
            name = "stillToProne";
        else if (m_movement.z < -0.5f)
            name = "backwardToProne";
        else if (m_movement.x > 0.5f)
            name = "rightToProne";
        else if (m_movement.x < -0.5f)
            name = "leftToProne";
        else
            name = "forwardToProne";
        trigger = m_template->getTriggerManager()->getTriggerByName(std::string(name), true);
    }
    if (current == 2)
        trigger = m_template->getTriggerManager()->getTriggerByName(std::string("fromProne"), true);

    if (trigger)
    {
        stopAllPlaying();
        trigger->update(*this);                             // Trigger vtable +0x4C
    }
    m_triggerPose = pose;
}


// Updates movement inputs, smoothing direction when dt is positive.
// Nonpositive dt copies direction, speed and look without smoothing.

void AnimationSystem::setTriggerMovement(float dt, const hfe::Vec3& direction, float speed,
                                         const hfe::Vec3& look)
{
    if (dt <= 0.0f)
    {
        m_movementSpeed = speed;
        m_look = look;
        m_movement = direction;
        return;
    }

    const float invDt = 1.0f / dt;
    const float previousX = m_movement.x * m_movementSpeed;
    const float previousY = m_movement.y * m_movementSpeed;
    const float previousZ = m_movementSpeed * m_movement.z;

    const float x = direction.x;
    const float z = direction.z;
    bool smooth = false;
    if ((x != 0.0f || z != 0.0f) && speed != 0.0f)
    {
        bool sameForward = true;
        if (fabsf(z) > 0.9f)
            sameForward = (signOf(z) == signOf(m_movement.z));
        if (sameForward)
        {
            if (fabsf(x) > 0.9f && signOf(x) != signOf(m_movement.x))
                smooth = false;
            else
                smooth = true;
        }
    }

    if (smooth)
    {
        const float rate = dt * *anim_smoothSet;
        float blend = 0.0f;
        if (rate >= 0.0f)
        {
            blend = rate;
            if (rate > 1.0f)
                blend = 1.0f;
        }
        const float keep = 1.0f - blend;
        float newZ = blend * z + keep * m_movement.z;
        float newX = x * blend + m_movement.x * keep;
        const float lengthSq = newZ * newZ + newX * newX;
        if (fabsf(lengthSq - 1.0f) >= FLT_EPSILON)
        {
            if (fabsf(lengthSq) >= 1.4210855e-14f)           // FLT_EPSILON squared
            {
                const float inv = 1.0f / sqrtf(lengthSq);
                newX = newX * inv;
                newZ = inv * newZ;
            }
            else
            {
                newX = 0.0f;
                newZ = 0.0f;
            }
        }
        m_movement.x = newX;
        m_movement.y = direction.y;
        m_movement.z = newZ;
    }
    else
    {
        m_movement.x = x;
        m_movement.z = z;
        m_movement.y = direction.y;
    }

    m_movementSpeed = speed;
    m_look = look;
    m_movementRate.z = (previousZ - speed * m_movement.z) * invDt;
    m_movementRate.y = (previousY - speed * m_movement.y) * invDt;
    m_movementRate.x = (previousX - speed * m_movement.x) * invDt;
}


// Synchronizes looping movement players by list order.
// Unpaired local players use the last matched time; anim_skipSync disables synchronization.

void AnimationSystem::syncMovementAnimations(const IAnimationSystem* other)
{
    if (*anim_skipSync)
        return;

    const AnimationSystem* const source = static_cast<const AnimationSystem*>(other);
    float time = -1.0f;
    std::list<BundlePlayer*>::iterator mine = m_players.begin();
    for (std::list<BundlePlayer*>::const_iterator theirs = source->m_players.begin();
         theirs != source->m_players.end(); ++theirs)
    {
        const BundlePlayer* const player = *theirs;
        if (!player->m_isMovement)
            continue;
        time = player->m_time;
        while (mine != m_players.end())
        {
            if ((*mine)->m_isMovement)
            {
                (*mine)->m_time = time;
                ++mine;
                break;
            }
            ++mine;
        }
    }

    if (time >= 0.0f)
    {
        for (; mine != m_players.end(); ++mine)
        {
            if ((*mine)->m_isMovement)
                (*mine)->m_time = time;
        }
    }
}

// Returns the accumulated listing by value.
std::string AnimationSystemTemplateManager::list()
{
    std::stringstream output;
    for (auto it = m_templates.begin(); it != m_templates.end(); ++it)
        output << it->first << "\n";
    return output.str();
}

} // namespace anim
} // namespace dice
