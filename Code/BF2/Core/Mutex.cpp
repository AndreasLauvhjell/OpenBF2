// Opaque mutex ownership with a one-pointer public layout; source path [INFERRED].
// The POSIX implementation follows the original; the Windows primitive is a chosen platform adaptation.

#include "Mutex.h"

#if defined(_WIN32)
#  include <windows.h>
#else
#  include <pthread.h>
#endif

#include <new>

namespace dice {
namespace hfe {

// Allocates and initializes the platform mutex payload.
Mutex::Mutex()
{
#if defined(_WIN32)
    // not recovered — the Windows stand-in. Same shape: heap payload, no attributes, no error check.
    CRITICAL_SECTION* p = static_cast<CRITICAL_SECTION*>(::operator new(sizeof(CRITICAL_SECTION)));
    InitializeCriticalSection(p);
    m_mutex = p;
#else
    // the allocation size is A literal 0x18 in the binary, not sizeof(pthread_mutex_t). On the
    // build dice used those agreed; on another they need not. Written as sizeof() here because a
    // hardcoded 24 would be silently wrong on any platform whose mutex is larger.
    pthread_mutex_t* p = static_cast<pthread_mutex_t*>(::operator new(sizeof(pthread_mutex_t)));
    pthread_mutex_init(p, 0);       // null attributes — the binary passes 0
    m_mutex = p;
#endif
}


// Destroys the owned platform mutex when present.
Mutex::~Mutex()
{
    if (m_mutex == 0)               // the null check is in the binary, not defensive padding
        return;

#if defined(_WIN32)
    CRITICAL_SECTION* p = static_cast<CRITICAL_SECTION*>(m_mutex);
    DeleteCriticalSection(p);
    ::operator delete(p);
#else
    pthread_mutex_t* p = static_cast<pthread_mutex_t*>(m_mutex);
    pthread_mutex_destroy(p);
    ::operator delete(p);
#endif
    m_mutex = 0;
}

// Acquires the platform mutex. Lock errors are not reported to the caller.
void Mutex::lock()
{
#if defined(_WIN32)
    EnterCriticalSection(static_cast<CRITICAL_SECTION*>(m_mutex));
#else
    pthread_mutex_lock(static_cast<pthread_mutex_t*>(m_mutex));
#endif
}

// The same shape.
void Mutex::unlock()
{
#if defined(_WIN32)
    LeaveCriticalSection(static_cast<CRITICAL_SECTION*>(m_mutex));
#else
    pthread_mutex_unlock(static_cast<pthread_mutex_t*>(m_mutex));
#endif
}

} // namespace hfe
} // namespace dice
