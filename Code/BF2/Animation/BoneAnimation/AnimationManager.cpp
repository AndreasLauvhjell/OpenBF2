// Animation resource management and deferred loading.

#include "AnimationManager.h"
#include <sstream>

#include <cstdio>       // [TEMP] fprintf/fflush - the postLoad sweep tracer
#include "BoneAnimation.h"

// [TEMP] [CHOSEN, not recovered] - the postLoad invocation counter, at file scope so the bonestage
// probe can report it from the abort path. [RETIRE-WHEN: bone-staging-index]
namespace { int g_debugPostLoadCalls = 0; }

namespace dice {
namespace anim {

AnimationManager* animationManager = 0;
// Starts with animation loading deferred and no active animation.

AnimationManager::AnimationManager()
    : m_activeAnimation(0)
    , m_deferLoading(true)
{
}

// Non-owning interface: fixed reference counts, no deletion, and no supported query interfaces.
void  AnimationManager::addRef()                          { }
int   AnimationManager::getRef()                          { return 1; }
int   AnimationManager::release()                         { return 0; }
void* AnimationManager::queryInterface(unsigned int) const { return 0; }


// Unregisters the file-change listener, then destroys all animations.

AnimationManager::~AnimationManager()
{
    hfe::io::fileChangeManager->removeListener(this);

    for (AnimationMap::iterator it = m_animations.begin(); it != m_animations.end(); ++it)
    {
        if (it->second.animation)
            delete it->second.animation;
    }
    m_animations.clear();
}


// Returns the animation cached under name, creating an unloaded entry when loading is deferred.

BoneAnimation* AnimationManager::getAnimation(const std::string& name)
{
    AnimationMap::iterator it = m_animations.find(name);
    if (it != m_animations.end())
    {
        it->second.wanted = true;                  // the mark, on the hit path too
        m_activeAnimation = it->second.animation;
        return m_activeAnimation;
    }

    BoneAnimation* animation = new BoneAnimation();          // operator new(0x50)
    if (!m_deferLoading)
        animation->load(name);                     // skipped while deferred - see above

    m_animations.insert(std::make_pair(name, AnimationHolder(true, animation)));
    m_activeAnimation = animation;
    return animation;
}


// Counts pending animations for deferred loading; returns zero when deferral is disabled.

int AnimationManager::getNumAnimationsToLoad()
{
    if (!m_deferLoading)
        return 0;

    int count = 0;
    for (AnimationMap::iterator it = m_animations.begin(); it != m_animations.end(); ++it)
    {
        if (it->second.wanted)
            ++count;
    }
    return count;
}


// Removes unwanted animations, loads requested entries, then clears their wanted flags.

void AnimationManager::postLoad()
{
    if (!m_deferLoading)
        return;

    // [TEMP] [CHOSEN, not recovered] Logs animation sweep state for the staging-index investigation.
    // [RETIRE-WHEN: bone-staging-index] Remove after validating animation lifetime across repeated level loads.
    const int callNumber = ++g_debugPostLoadCalls;
    int swept = 0;
    const size_t before = m_animations.size();

    // Pass 1 - the sweep.
    for (AnimationMap::iterator it = m_animations.begin(); it != m_animations.end(); )
    {
        if (it->second.wanted)
        {
            ++it;
            continue;
        }
        if (it->second.animation)
            delete it->second.animation;
        ++swept;                                   // [TEMP]
        m_animations.erase(it++);                  // dice restarts from begin(); same effect
    }

    // [TEMP] - see above.
    std::fprintf(stderr, "[TEMP][ANIMSWEEP] postLoad #%d: %u registered, %d swept, %u to load.\n",
                 callNumber, static_cast<unsigned>(before), swept,
                 static_cast<unsigned>(m_animations.size()));
    std::fflush(stderr);

    // Pass 2 - the load, and the mark reset.
    for (AnimationMap::iterator it = m_animations.begin(); it != m_animations.end(); ++it)
    {
        it->second.animation->load(it->first);     // unguarded, and the result is discarded
        it->second.wanted = false;
    }
}


// Returns zero without releasing animation storage. postLoad owns the collection of unused animations.

int AnimationManager::release(BoneAnimation* /*animation*/)
{
    return 0;
}

// five bytes - empty. The manager is registered with the file-change watcher (the
// destructor unregisters it), so hot-reload is wired up and then declines to do anything.
void AnimationManager::onFileChange(const std::string& /*path*/, unsigned int /*what*/) { }

// A miss leaves the active animation alone - the one setActive* in this whole
// subsystem that does not clear on a miss. Its three siblings all do.
void AnimationManager::setActiveAnimationFile(const std::string& name)
{
    AnimationMap::iterator it = m_animations.find(name);
    if (it != m_animations.end())
        m_activeAnimation = it->second.animation;
}


// Returns the active animation's filename, or a stable no-active-animation string.

const std::string& AnimationManager::getActiveAnimationFile()
{
    static const std::string s_none("no active animation");

    for (AnimationMap::iterator it = m_animations.begin(); it != m_animations.end(); ++it)
    {
        if (it->second.animation == m_activeAnimation)
            return it->first;
    }
    return s_none;
}

const std::string& AnimationManager::getName(BoneAnimation* animation)
{
    static const std::string notFound("no animation");

    for (AnimationMap::iterator it = m_animations.begin(); it != m_animations.end(); ++it)
    {
        if (it->second.animation == animation)
            return it->first;
    }
    return notFound;
}


// [TEMP] [CHOSEN, not recovered] Returns the registered key for animation, or null, for staging diagnostics.
// [RETIRE-WHEN: bone-staging-index] Remove after resolving and validating animation registration during staging.

const std::string* AnimationManager::debugFindRegisteredName(const BoneAnimation* animation) const
{
    for (AnimationMap::const_iterator it = m_animations.begin(); it != m_animations.end(); ++it)
    {
        if (it->second.animation == animation)
            return &it->first;
    }
    return 0;
}

int AnimationManager::debugPostLoadCalls() const
{
    return g_debugPostLoadCalls;
}

// Returns the animation listing by value.
std::string AnimationManager::listAnimations() const
{
    std::stringstream output;
    for (AnimationMap::const_iterator it = m_animations.begin(); it != m_animations.end(); ++it)
        output << it->first << "\n";
    return output.str();
}

} // namespace anim
} // namespace dice
