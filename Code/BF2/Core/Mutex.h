// Owns an opaque, heap-allocated platform mutex. Header path [INFERRED].
// The wrapper stays pointer-sized because engine classes embed it by value.
// lock/unlock expose no error result. Keep platform storage behind this interface.

#ifndef CORE_MUTEX_H                // [INFERRED] guard style
#define CORE_MUTEX_H

namespace dice {
namespace hfe {

// complete. sizeof == 4.
class Mutex
{
public:
    Mutex();
    ~Mutex();

    void lock();
    void unlock();

private:
    void* m_mutex;      // +0x00 — heap-allocated, 24 bytes (a pthread_mutex_t on the recovered build)

    // Not recovered as deleted in the binary, but a Mutex that is copied would double-free its
    // payload in the destructor. Suppressed rather than left implicit.
    Mutex(const Mutex&);
    Mutex& operator=(const Mutex&);
};

} // namespace hfe
} // namespace dice

#endif
