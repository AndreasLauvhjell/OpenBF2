// [CHOSEN, not recovered] gameswf allocation adapters using the engine heap.
// Host-facing strings use the separate MSVC runtime heap. Reallocation allocates, copies min(old,new)
// and frees because the imported engine interface supplies no realloc.

#include "../RendDX9Abi/BfMemory.h"
#include "../RendDX9Abi/Frontier.h"

#include <string.h>   // memcpy
#include <stdlib.h>   // abort

namespace swiffplayer {
namespace mem {

void* tuMalloc(unsigned int size)
{
    // engineAlloc -> bf_malloc(size, g_memoryNameId). The bucket id is the "externDll" tag
    // registered by initDll (SwiffPlayerAbi/PluginEntry.cpp); until then it is 0, exactly as the
    // shipped image's static-initialiser allocations land in bucket 0 before initDll runs.
    return ::renddx9abi::engineAlloc(size ? size : 1u);
}

void* tuRealloc(void* oldPtr, unsigned int newSize, unsigned int oldSize)
{
    if (newSize == 0)
    {
        ::renddx9abi::engineFree(oldPtr);
        return 0;
    }
    void* p = ::renddx9abi::engineAlloc(newSize);
    if (p && oldPtr)
    {
        const unsigned int n = (oldSize < newSize) ? oldSize : newSize;
        ::memcpy(p, oldPtr, n);
    }
    if (oldPtr)
        ::renddx9abi::engineFree(oldPtr);
    return p;
}

void tuFree(void* oldPtr)
{
    if (oldPtr)
        ::renddx9abi::engineFree(oldPtr);
}

// tu_error_exit: fatal, must not return. Routes to the shared frontier log so an early-boot gameswf
// fatal is captured like any other stop, then aborts. Not a silent stub - it is loud by contract.
void tuErrorExit(int errorCode, const char* errorMessage)
{
    ::renddx9abi::logf("[FATAL] gameswf tu_error_exit(code=%d): %s", errorCode,
                       errorMessage ? errorMessage : "(null)");
    ::renddx9abi::logClose();
    ::abort();
}

} // namespace mem
} // namespace swiffplayer
