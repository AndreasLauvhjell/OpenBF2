// Path:     [CHOSEN, not recovered] - see Msvcr71Heap.h.

#include "Msvcr71Heap.h"
#include "PluginAbi.h"

#if BF2_PLUGIN_ABI_NATIVE
#include <cstdlib>
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace renddx9abi {

#if !BF2_PLUGIN_ABI_NATIVE
namespace {

typedef void* (__cdecl* MallocFn)(size_t);
typedef void  (__cdecl* FreeFn)(void*);

MallocFn s_malloc = 0;
FreeFn   s_free   = 0;
bool     s_tried  = false;

void resolveOnce()
{
    if (s_tried)
        return;
    s_tried = true;

    // GetModuleHandle, not LoadLibrary. If msvcr71.dll is not already mapped then this process is
    // not the one this plugin targets, and pulling a second copy of an ancient CRT into it would
    // create a third heap rather than fix anything.
    HMODULE crt = ::GetModuleHandleA("msvcr71.dll");
    if (!crt)
        return;

    s_malloc = reinterpret_cast<MallocFn>(::GetProcAddress(crt, "malloc"));
    s_free   = reinterpret_cast<FreeFn>  (::GetProcAddress(crt, "free"));
    if (!s_malloc || !s_free)
    {
        s_malloc = 0;
        s_free   = 0;
    }
}

} // anonymous namespace
#endif

void* crtMalloc(size_t size)
{
#if BF2_PLUGIN_ABI_NATIVE
    return std::malloc(size);
#else
    resolveOnce();
    return s_malloc ? s_malloc(size) : 0;
#endif
}

void crtFree(void* p)
{
#if BF2_PLUGIN_ABI_NATIVE
    std::free(p);
#else
    resolveOnce();
    if (s_free && p)
        s_free(p);
#endif
}

bool crtHeapAvailable()
{
#if BF2_PLUGIN_ABI_NATIVE
    return true;
#else
    resolveOnce();
    return s_malloc != 0 && s_free != 0;
#endif
}

} // namespace renddx9abi
