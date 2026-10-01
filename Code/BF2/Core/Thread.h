// Thread interface with an owned platform control block; header path [INFERRED].
// Priority is a nested type; observed values and the current control-block layout are declared below.

#ifndef CORE_THREAD_H           // [INFERRED] guard style
#define CORE_THREAD_H

namespace dice {
namespace hfe {

// Thread owns a platform-specific control block while preserving its compact public layout.
// start() ignores name on both platforms. Windows applies priority; Linux ignores it.
// The Windows control block stores both a handle and ID; the POSIX block stores pthread_t.
// stop() joins; callers must arrange for run() to finish because there is no cancellation.

class Thread
{
public:
    // Win32 priorities. Normal preserves CreateThread's default without a setter call;
    // out-of-range values explicitly select normal priority. The ELF ignores this argument.
    enum Priority
    {
        Priority_Idle         = 0,   // SetThreadPriority(-15)  THREAD_PRIORITY_IDLE
        Priority_BelowNormal  = 1,   // SetThreadPriority(-1)   THREAD_PRIORITY_BELOW_NORMAL
        Priority_Normal       = 2,   // no call at all — see above
        Priority_AboveNormal  = 3,   // SetThreadPriority(+1)   THREAD_PRIORITY_ABOVE_NORMAL
        Priority_TimeCritical = 4    // SetThreadPriority(+15)  THREAD_PRIORITY_TIME_CRITICAL
    };
    // [INFERRED] enumerator names — the values and their Win32 mappings are read, the identifiers
    // are chosen to match the constants they select. No symbol names them.

    Thread();
    virtual ~Thread();

    // Thread entry point supplied by subclasses. [UNRESOLVED] Original method name.
    virtual void run() = 0;                         // +0x08

    // Starts the thread and reports success. name is ignored; priority is also ignored on Linux.
    // [CHOSEN] Boolean return type.
    bool start(const char* name, Priority);
    // `bool`, not the `void` this line used to say - the Linux body returns pthread_join's
    // success and the Win32 body `mov al,1`; Thread.cpp always defined it as bool and the mismatch
    // only went unnoticed because the Win32 arm was not compiled. [CHOSEN] no caller tests it.
    bool stop();

private:
    // 8 bytes total: vptr + this. The block behind it is platform-sized — see the header block.
    struct Impl;
    Impl* m_impl;                                   // +0x04, operator new(8) on Linux
};

} // namespace hfe
} // namespace dice

#endif // CORE_THREAD_H
