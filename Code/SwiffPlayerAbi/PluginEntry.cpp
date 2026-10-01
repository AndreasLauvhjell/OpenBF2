// [PLATFORM] SwiffPlayer host handshake using HostString for the selected host ABI.
// Registers the externDll memory bucket before assertions, then acquires services in shipped order.
// Registers its one module before initializing it. LoadStat, Game and AIConnection are optional
// members of the shared service lookup sequence; diagnostics use this image's configuration.
// [RETIRE-WHEN: RW-49] Consolidate with the shared DllModule when per-image ABI types are supported.

#include "../RendDX9Abi/HfeAbi.h"
#include "../RendDX9Abi/BfMemory.h"
#include "../RendDX9Abi/Msvcr71Heap.h"
#include "../RendDX9Abi/Frontier.h"

#include <cassert>

namespace dice {
namespace hfe {

// The nineteen globals this file fills, in the binary's order. Addresses are the shipped image's
// (dword_100EFE90.. in SwiffPlayer.dll). Typed IBase* throughout for the reason RendDX9's file
// gives: getSingleton hands back an IBase* and the binary stores it with no adjustment; a concrete
// type here could let a static_cast insert a this-adjustment under MI and diverge invisibly.
IClassManager* classManager           = 0;   // adjacent (the store target g_hfeClassManager)

IBase* settings                       = 0;   // 0x100EFE90  "SettingsRepostitory" (dice's typo)
IBase* mainConsoleObjects             = 0;
IBase* mainConsole                    = 0;
IBase* fileManager                    = 0;
IBase* fileChangeManager              = 0;
IBase* bfGlobal_callback              = 0;
IBase* bfGlobal_callbackData          = 0;
IBase* debugOn                        = 0;
IBase* debugModules                   = 0;
IBase* profilerClient                 = 0;
IBase* g_system                       = 0;
IBase* loadStat                       = 0;   // 0x100EFEB8   not required
IBase* eventManager                   = 0;   // 0x100EFEAC (g_EventManager)
IBase* frameEventManager              = 0;
IBase* objectManager                  = 0;
IBase* objectTemplateManager          = 0;
IBase* playerManager                  = 0;
IBase* game                           = 0;   // 0x100EFEEC   not required
IBase* aiConnection                   = 0;   // 0x100EFEF0   not required

// Null-terminated module table constructed by the Swiff wrapper before initDll.
// Both lifecycle exports traverse this shared table.
extern IModule* dllmodules[];

// 0x48A6E0 in the ELF; MSVC inlines it into both exports. The function-local static is dice's.
int getNoModules()
{
    static int no = 0;
    if (no == 0 && dllmodules[0] != 0)
    {
        int i = 1;
        int result;
        do
        {
            result = i;
        }
        while (dllmodules[i++] != 0);
        no = result;
    }
    return no;
}


extern "C" bool initDll(IClassManager* pClsMgr)
{
    // sub_1006DB40("externDll") - the first statement, ahead of the assert. Names this image's
    // Memory.dll bucket. See RendDX9Abi/BfMemory.h; the tag differs, the mechanism does not.
    renddx9abi::memoryName("externDll");

    // Ours, not dice's: open the instrument before anything can trip over.
    renddx9abi::logOpen();
    renddx9abi::logf("initDll(classManager=0x%08X) - replacement SwiffPlayer",
                     renddx9abi::addr(pClsMgr));

    // the CRT-heap check is ours and not optional for the retail flavour. Native
    // mode uses the shared /MD CRT, as selected by Msvcr71Heap.cpp. Retail names longer than 15 chars are
    // heap-allocated by vc71::string and later freed by MSVCP71 inside BF2.exe. If msvcr71.dll could
    // not be found those frees would go to the wrong heap - so refuse rather than corrupt the host.
    if (!renddx9abi::crtHeapAvailable())
    {
        renddx9abi::logf("initDll: msvcr71.dll malloc/free could not be resolved. The string ABI "
                         "cannot be honoured; refusing to register. Returning false.");
        return false;
    }

    // attested line - Code/BF2/Core/DllModule.cpp:72. The stringised expression is the bare pointer,
    // so dice wrote assert(pClsMgr).
    assert(pClsMgr);

    classManager = pClsMgr;

    // The nineteen pulls, in the binary's order
    settings              = classManager->getSingleton(renddx9abi::HostString("SettingsRepostitory"));
    mainConsoleObjects    = classManager->getSingleton(renddx9abi::HostString("MainConsoleObjects"));
    mainConsole           = classManager->getSingleton(renddx9abi::HostString("MainConsole"));
    fileManager           = classManager->getSingleton(renddx9abi::HostString("FileManager"));
    fileChangeManager     = classManager->getSingleton(renddx9abi::HostString("FileChangeManager"));
    bfGlobal_callback     = classManager->getSingleton(renddx9abi::HostString("BfGlobal_callback"));
    bfGlobal_callbackData = classManager->getSingleton(renddx9abi::HostString("BfGlobal_callbackData"));
    debugOn               = classManager->getSingleton(renddx9abi::HostString("DebugOn"));
    debugModules          = classManager->getSingleton(renddx9abi::HostString("DebugModules"));
    profilerClient        = classManager->getSingleton(renddx9abi::HostString("Profiler"));
    g_system              = classManager->getSingleton(renddx9abi::HostString("System"));
    loadStat              = classManager->getSingleton(renddx9abi::HostString("LoadStat"));
    eventManager          = classManager->getSingleton(renddx9abi::HostString("EventManager"));
    frameEventManager     = classManager->getSingleton(renddx9abi::HostString("FrameEventManager"));
    objectManager         = classManager->getSingleton(renddx9abi::HostString("ObjectManager"));
    objectTemplateManager = classManager->getSingleton(renddx9abi::HostString("ObjectTemplateManager"));
    playerManager         = classManager->getSingleton(renddx9abi::HostString("PlayerManager"));
    game                  = classManager->getSingleton(renddx9abi::HostString("Game"));
    aiConnection          = classManager->getSingleton(renddx9abi::HostString("AIConnection"));

    // The sixteen that are required (LoadStat, Game, AIConnection deliberately excluded)
    const bool ok =
           settings              != 0
        && mainConsoleObjects    != 0
        && mainConsole           != 0
        && fileManager           != 0
        && fileChangeManager     != 0
        && bfGlobal_callback     != 0
        && bfGlobal_callbackData != 0
        && debugOn               != 0
        && debugModules          != 0
        && profilerClient        != 0
        && g_system              != 0
        && objectManager         != 0
        && objectTemplateManager != 0
        && playerManager         != 0
        && eventManager          != 0
        && frameEventManager     != 0;

    renddx9abi::logf("initDll: 19 singletons pulled, required-16 chain = %s", ok ? "true" : "false");
    if (!ok)
    {
        static const char* const names[16] =
        {
            "SettingsRepostitory", "MainConsoleObjects", "MainConsole", "FileManager",
            "FileChangeManager", "BfGlobal_callback", "BfGlobal_callbackData", "DebugOn",
            "DebugModules", "Profiler", "System", "ObjectManager",
            "ObjectTemplateManager", "PlayerManager", "EventManager", "FrameEventManager"
        };
        IBase* const values[16] =
        {
            settings, mainConsoleObjects, mainConsole, fileManager,
            fileChangeManager, bfGlobal_callback, bfGlobal_callbackData, debugOn,
            debugModules, profilerClient, g_system, objectManager,
            objectTemplateManager, playerManager, eventManager, frameEventManager
        };
        for (int i = 0; i < 16; ++i)
            if (!values[i])
                renddx9abi::logf("initDll:   MISSING required singleton \"%s\"", names[i]);
    }

    // Pass one: every module joins the host's registry; pass two: only then is any started
    for (int i = 0; dllmodules[i] != 0; ++i)
        classManager->registerServer(dllmodules[i]);
    for (int i = 0; dllmodules[i] != 0; ++i)
        dllmodules[i]->init();

    renddx9abi::logf("initDll: returning %s", ok ? "true" : "false");
    return ok;
}


extern "C" void deinitDll()
{
    renddx9abi::logf("deinitDll: %d modules", getNoModules());

    for (int i = getNoModules() - 1; i >= 0; --i)
        dllmodules[i]->close();

    for (int i = getNoModules() - 1; i >= 0; --i)
    {
        classManager->unregisterServer(dllmodules[i]);
        dllmodules[i]->release();
        dllmodules[i] = 0;   // reproduced: nulls the slot (harmless - nothing calls getNoModules after)
    }

    renddx9abi::logf("deinitDll: done");
    renddx9abi::logClose();
}

} // namespace hfe
} // namespace dice
