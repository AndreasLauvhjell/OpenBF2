// [INFERRED] Source placement.
// [FRONTIER] Remaining methods stop by name until implemented.
// [TEMP] Temporary implementation boundaries remain.
// [CHOSEN, not recovered] Temporary implementation boundaries remain.
// [RETIRE-WHEN: aidll-01-adapters] Complete and validate the remaining BFTimer operations.

#include "BFTimer.h"

#include "AIFrontier.h"

#include "../../AIDLL/AI/Main/CommonSingletonServer.h"
#include "../../../Core/ClassManager.h"
#include "../../../Core/System.h"   // namespace WorldPref
#if defined(_WIN32)
#include <windows.h>                // GetSystemTimeAsFileTime - the chosen Win32 arm of bfTimerNow
#else
#include <sys/time.h>               // gettimeofday - the ELF's call ( hands m_wallClock to it)
#endif

namespace dice {
namespace hfe {

extern ClassManager* classManager;

namespace ai {

// First wins in, exact match out.
IBFTimer::IBFTimer()
{
    if (!CommonSingletonServer::m_iBFTimer)
        classManager->registerSimpleSingleton(std::string("IBFTimer"), this);
}

IBFTimer::~IBFTimer()
{
    if (CommonSingletonServer::m_iBFTimer == this)
        classManager->unregisterSimpleSingleton(std::string("IBFTimer"));
}

BFTimer::BFTimer()
    : m_paused(false)
    , m_resetMark(0.0)
    , m_wallClock(new BFTimerWallClock)
{
    bfTimerNow(m_wallClock);
    reset();
}

// the vptr reset, `operator delete` on the wall
// clock, then the base destructor. It really does free the eight bytes the ctor allocated.
BFTimer::~BFTimer()
{
    delete m_wallClock;
}


// The seven virtuals and the two non-virtual helpers, all read from the binary.

// - `push ebp / mov ebp, esp / pop ebp / jmp WorldPref::getWorldTime`. A tail
// jump: the timer owns no game clock, it forwards to WorldPref.
double BFTimer::getGameTime() const
{
    return WorldPref::getWorldTime();
}

// - the same shape, to WorldPref::getTickTime.
double BFTimer::getTimeIncrease() const
{
    return WorldPref::getTickTime();
}

// Returns elapsed wall-clock time in seconds, including the microsecond fraction.
double BFTimer::getUpdateTime() const
{
    BFTimerWallClock now;
    bfTimerNow(&now);
    return (now.tv_sec + now.tv_usec * 1e-6)
         - (m_wallClock->tv_sec + m_wallClock->tv_usec * 1e-6);
}

// Returns the system wall-clock time.
double BFTimer::getWallClockTime() const
{
    return g_system->getWallClockTime();
}

// - two tail jumps, one per branch.
void BFTimer::setPause(bool paused)
{
    if (paused)
        pause();
    else
        unpause();
}

// - `movzx eax, byte ptr [eax+4]`.
bool BFTimer::getPause()
{
    return m_paused;
}

void BFTimer::reset()
{
    m_paused = false;
    m_resetMark = 0.0;
    bfTimerNow(m_wallClock);
}

// each. not virtual - absent from the vtable, and setPause is
// their only caller.
void BFTimer::pause()
{
    m_paused = true;
}

void BFTimer::unpause()
{
    m_paused = false;
}
} // namespace ai
} // namespace hfe
} // namespace dice

// [CHOSEN, not recovered] Produces seconds and microseconds using gettimeofday on ELF
// and a FILETIME epoch conversion on Windows. The original Windows clock call is unknown.
// [RETIRE-WHEN: aidll-01-adapters] Replace the Windows clock route when its implementation is confirmed.
namespace dice {
namespace hfe {
namespace ai {

void bfTimerNow(BFTimerWallClock* out)
{
#if defined(_WIN32)
    FILETIME ft;
    ::GetSystemTimeAsFileTime(&ft);

    unsigned long long ticks = (static_cast<unsigned long long>(ft.dwHighDateTime) << 32)
                             | ft.dwLowDateTime;             // 100 ns units since 1601-01-01
    ticks -= 116444736000000000ULL;                          // > since 1970-01-01

    out->tv_sec  = static_cast<long>(ticks / 10000000ULL);
    out->tv_usec = static_cast<long>((ticks % 10000000ULL) / 10ULL);
#else
    // [PLATFORM] The ELF's call, as read: the eight-byte block is a struct timeval and is handed
    // straight to gettimeofday. Copied through a real timeval here rather than aliased, since the
    // native port's BFTimerWallClock is only layout-compatible by construction, not by declaration.
    struct timeval now;
    ::gettimeofday(&now, 0);
    out->tv_sec  = static_cast<long>(now.tv_sec);
    out->tv_usec = static_cast<long>(now.tv_usec);
#endif
}

} // namespace ai
} // namespace hfe
} // namespace dice
