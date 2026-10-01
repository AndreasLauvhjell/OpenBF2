// Loads engine plugins for client and dedicated images; source path [INFERRED].
// Windows unload calls optional deinitDll and reports exceptions. ELF unload only closes the library.

#include "PluginDll.h"

#include <cstring>
#include <string>
#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>      // dlopen / dlsym / dlclose - what the ELF's LoadLibrary trio wraps
#endif

#include "ClassManager.h"

#if !defined(_WIN32)
// [PLATFORM] Global-namespace library wrappers for ELF; placement is [INFERRED].
// Loading strips the optional Windows-style prefix, requires an extension and substitutes lib<stem>.so.
// Symbol lookup uses dlsym; close returns false for null. The initial two-byte comparison is unguarded.
void* LoadLibrary(const char* name)
{
    if (std::memcmp(name, ".\\", 2) == 0)
        name += 2;
    const char* const dot = std::strrchr(name, '.');
    if (!dot)
        return 0;
    std::string file = std::string("lib") + std::string(name, dot);
    file.append(".so");
    return ::dlopen(file.c_str(), RTLD_LAZY);
}

void* GetProcAddress(void* handle, const char* name)
{
    return ::dlsym(handle, name);
}

bool FreeLibrary(void* handle)
{
    if (handle)
        return ::dlclose(handle) == 0;
    return false;
}
#endif // !_WIN32

namespace dice {
namespace hfe {

class IClassManager; // the PE export's attested interface type; ClassManager is layout-compatible

extern ClassManager* classManager;
namespace memory { void memory_update_modules(const char*); }

typedef bool (__cdecl *InitDllFn)(IClassManager*);
typedef void (__cdecl *DeinitDllFn)();

#if defined(_MSC_VER)
void* __fastcall LoadLibraryBf(const char* name)
#else
void* LoadLibraryBf(const char* name)
#endif
{
    // The missing explicit terminator is a shipped defect: a name of 128 bytes or more lets the
    // following string construction read past this buffer.  Preserve it rather than silently
    // changing the loader's observable input handling.
    char copiedName[128];
    std::strncpy(copiedName, name, sizeof(copiedName));

    // The shared loader path uses a backslash; the Linux wrapper normalizes it before loading.
    const std::string path = std::string(".") + "\\" + copiedName;
#if defined(_WIN32)
    HMODULE module = ::LoadLibraryA(path.c_str());
#else
    void* module = ::LoadLibrary(path.c_str());               // the wrapper above
#endif
    if (!module)
        return 0;

    InitDllFn initDll = reinterpret_cast<InitDllFn>(::GetProcAddress(module, "initDll"));
    if (!initDll || !initDll(reinterpret_cast<IClassManager*>(classManager)))
    {
        ::FreeLibrary(module);
        return 0;
    }

    memory::memory_update_modules(name);
    return module;
}

#if defined(_MSC_VER)
bool __fastcall FreeLibraryBf(void* handle, const char* diagnosticName)
#else
bool FreeLibraryBf(void* handle, const char* diagnosticName)
#endif
{
#if defined(_WIN32)
    try
    {
        HMODULE module = static_cast<HMODULE>(handle);
        if (module)
        {
            DeinitDllFn deinitDll =
                reinterpret_cast<DeinitDllFn>(::GetProcAddress(module, "deinitDll"));
            if (deinitDll)
                deinitDll();
        }

        // The retail body calls FreeLibrary even when handle is null.
        return ::FreeLibrary(module) != FALSE;
    }
    catch (...)
    {
        ::MessageBoxA(0, diagnosticName, "DLL unload error:", 0x16);
        return false;
    }
#else
    // : the bare wrapper call. No deinitDll lookup, no diagnostic, and the
    // second argument is never read.
    (void)diagnosticName;
    return ::FreeLibrary(handle);
#endif
}

} // namespace hfe
} // namespace dice
