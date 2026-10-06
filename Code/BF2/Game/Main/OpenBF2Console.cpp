// [CHOSEN, not recovered] The `openbf2.*` console object. This is an OpenBF2 extension, not a
// recovered Battlefield 2 1.50 command; no retail image defines it. `openbf2.addModDll` loads a
// native module at run time - a mod, or an integration shim such as bf2hub's bf2hub.dll/libbf2hub.so.
// Issue: B2BF/OpenBF2#39 (approach discussed there before implementation).
//
// Safety model (discussed in the issue):
//   - Dedicated-server image only. The whole binding compiles out of the client and editor, so a
//     downloaded map cannot reach it on a player's machine.
//   - It refuses any call dispatched from a mounted-archive script, so a map's or mod's bundled
//     .con files cannot silently load native code. Operator-controlled callers - a loose server
//     .con, rcon, Python, or the server console - are allowed (see isDispatchingArchiveScript).
//
// Loader contract (lenient, so both plugin styles work):
//   - A module exporting initDll(IClassManager*) is initialised through the same handshake the
//     engine uses for its own plugins (Core/PluginDll.cpp); a false return unloads it and fails.
//   - A module without initDll is kept loaded, because its DllMain has already run. That is what a
//     DllMain-based integration such as bf2hub needs: bf2hub's DllMain (server path, gated on the
//     host image name containing "w32ded") patches the engine's dice_py import slots Py_InitModule4
//     and PyImport_ImportModule, so bf2hublib and bf2.bf2hub are injected when the engine registers
//     its `host` module and imports `bf2`. That only works if the module is loaded BEFORE Python
//     initialization - i.e. from ServerSettings.con or Init.con, which run before GameServer::init
//     calls pythonHost()->initialize(), not from a map script that runs afterward.
//   - This loader does not itself reroute sockets. bf2hub's GameSpy/master redirection relied on an
//     install-time patch of the host's import table that a runtime load does not reproduce; OpenBF2
//     points the engine at the correct master-server address in source instead (see the issue).

#include "../../Core/ConsoleObjectMacro.h"

#if DEDICATED

// All STL and engine headers are included before <windows.h> so its min/max macros cannot break them.
#include "../../IO/Console/Console.h"

#include <string>
#include <vector>
#include <cstdio>

#if defined(_WIN32)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#else
#  include <dlfcn.h>
#endif

namespace dice {
namespace hfe {

class ClassManager;
extern ClassManager* classManager;   // Core/Globals.cpp - the host singleton handed to a plugin initDll.

namespace {

const char* const kOpenBf2ConsoleTag = "openbf2.console";

// Matches the initDll signature Core/PluginDll.cpp loads for engine plugins.
typedef bool (__cdecl* InitDllFn)(void*);

struct LoadedModule
{
    std::string requested;   // the name exactly as typed in the console
    std::string resolved;    // the platform path actually handed to the loader
    void*       handle;
};

// Function-local so it is constructed before any binding's static constructor runs and never
// becomes a load-order problem. It exists only to dedupe repeat loads and keep handles alive.
std::vector<LoadedModule>& loadedModules()
{
    static std::vector<LoadedModule> modules;
    return modules;
}

// Resolves the console argument to the platform's library path.
//   Windows: a bare name is loaded from the current directory (".\\name") rather than the system
//            DLL search order; a name containing a separator is used as given.
//   ELF:     a bare name becomes lib<stem>.so - matching retail bf2hub's libbf2hub.so, resolved
//            through the bf2 binary's $ORIGIN runpath - so the same `foo.dll` spelling works on
//            both platforms; a name containing a '/' is used as given.
std::string resolveModulePath(const std::string& name)
{
#if defined(_WIN32)
    if (name.find('/') != std::string::npos || name.find('\\') != std::string::npos)
        return name;
    return std::string(".\\") + name;
#else
    if (name.find('/') != std::string::npos)
        return name;
    std::string stem = name;
    const std::string::size_type dot = stem.rfind('.');
    if (dot != std::string::npos)
        stem.erase(dot);                       // drop a .dll/.so suffix before forming lib<stem>.so
    if (stem.compare(0, 3, "lib") == 0)
        stem.erase(0, 3);
    return std::string("lib") + stem + ".so";
#endif
}

std::string lastOsError()
{
#if defined(_WIN32)
    char buf[32];
    std::sprintf(buf, "error %lu", static_cast<unsigned long>(::GetLastError()));
    return std::string(buf);
#else
    const char* msg = ::dlerror();
    return msg ? std::string(msg) : std::string("unknown error");
#endif
}

void* openModule(const std::string& path)
{
#if defined(_WIN32)
    return ::LoadLibraryA(path.c_str());
#else
    return ::dlopen(path.c_str(), RTLD_LAZY);
#endif
}

void closeModule(void* handle)
{
#if defined(_WIN32)
    ::FreeLibrary(static_cast<HMODULE>(handle));
#else
    ::dlclose(handle);
#endif
}

void* moduleSymbol(void* handle, const char* symbol)
{
#if defined(_WIN32)
    return reinterpret_cast<void*>(::GetProcAddress(static_cast<HMODULE>(handle), symbol));
#else
    return ::dlsym(handle, symbol);
#endif
}

// Loads the named module. Returns a human-readable status string (empty is never returned, so the
// outcome always reaches an operator over rcon or the server console). Also echoed to the server
// console so the result is visible when a script discards the return value.
std::string openBf2AddModDll(const std::string& name)
{
    std::string result;

    if (name.empty())
    {
        result = "openbf2.addModDll: no module name given";
    }
    else if (io::g_executingConsole && io::g_executingConsole->isDispatchingArchiveScript())
    {
        result = "openbf2.addModDll: refused '" + name + "' (archive-sourced command)";
    }
    else
    {
        std::vector<LoadedModule>& mods = loadedModules();
        bool already = false;
        for (std::vector<LoadedModule>::const_iterator it = mods.begin(); it != mods.end(); ++it)
        {
            if (it->requested == name)
            {
                already = true;
                break;
            }
        }

        if (already)
        {
            result = "openbf2.addModDll: '" + name + "' already loaded";
        }
        else
        {
            const std::string path = resolveModulePath(name);
            void* handle = openModule(path);
            if (!handle)
            {
                result = "openbf2.addModDll: could not load '" + path + "' (" + lastOsError() + ")";
            }
            else
            {
                InitDllFn initDll = reinterpret_cast<InitDllFn>(moduleSymbol(handle, "initDll"));
                if (initDll && !initDll(reinterpret_cast<void*>(classManager)))
                {
                    closeModule(handle);
                    result = "openbf2.addModDll: initDll refused '" + path + "'";
                }
                else
                {
                    LoadedModule entry;
                    entry.requested = name;
                    entry.resolved  = path;
                    entry.handle    = handle;
                    mods.push_back(entry);
                    result = std::string("openbf2.addModDll: loaded '") + path + "'"
                             + (initDll ? " (initDll ok)" : " (no initDll; DllMain only)");
                }
            }
        }
    }

    if (io::mainConsole)
        io::mainConsole->output(result);
    return result;
}

} // anonymous namespace

BF2_CONSOLE_OBJECT("openbf2", 1, 0)
BF2_CONSOLE_METHOD_1_RET_ACTIVE(addModDll, openBf2AddModDll, std::string, true,
                                std::string, kOpenBf2ConsoleTag)

// Handed to the console registry by ModuleBF2::init (Core/ModuleRegistry.cpp).
ConsoleObjectList& openBf2ConsoleObjects()
{
    return consoleBindingList(kOpenBf2ConsoleTag);
}

} // namespace hfe
} // namespace dice

#endif // DEDICATED
