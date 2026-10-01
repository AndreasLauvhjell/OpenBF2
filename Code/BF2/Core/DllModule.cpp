// Side-DLL initialization and shutdown; compile only into module images.
// initDll stores all host-service lookups before checking required services. LoadStat, Game and
// AIConnection are optional. It registers every module before initializing any of them.
// The return value reflects required service lookups; module init() return values are discarded.

#include "DllModule.h"

#include <cassert>            // assert(pClsMgr) - see the body
#include <string>

#include "Debug.h"              // DebugCallback, and the four debug globals' real types
#include "ClassManager.h"       // getSingleton / registerServer / unregisterServer, by slot
#include "IBase.h"              // release() on the way out
#include "IModule.h"            // init() / close()

namespace dice {
namespace hfe {

// Host globals populated by initDll. Match existing declarations for consistent linkage.
// Singleton pointers are copied without base adjustment; reinterpret_cast preserves those raw stores.
class ConsoleObjects;                       // Core/ConsoleObjects.cpp - not an IBase
class EventManager;
class IGame;
class IProfilerClient;
class LoadStat;                             // Core/LoadStat.h
class SettingsRepostitory;                  // dice's spelling, frozen - see Core/Globals.cpp
class System;

namespace io    { class FileChangeManager; class FileManager; class OldConsole; }
namespace world { class ObjectManager; class ObjectTemplateManager; class PlayerManager; }

extern ClassManager*         classManager;              // ds:0x8AC85A0 in bf2
extern SettingsRepostitory*  settings;
extern ConsoleObjects*       mainConsoleObjects;
// Use the declared debug-global types so modules bind to the host's exported symbols.
extern DebugCallback*        bfGlobal_callback;
extern void**                bfGlobal_callbackData;
extern bool*                 debugOn;
extern Hash<std::string, std::string, StringHashFunc>* debugModules;
extern IProfilerClient*      profilerClient;
extern System*               g_system;
extern LoadStat*             loadStat;
extern EventManager*         eventManager;
extern EventManager*         frameEventManager;
extern IGame*                game;

namespace io {
extern OldConsole*           mainConsole;
extern FileManager*          fileManager;
extern FileChangeManager*    fileChangeManager;
} // namespace io

namespace world {
extern ObjectManager*         objectManager;
extern ObjectTemplateManager* objectTemplateManager;
extern PlayerManager*         playerManager;
} // namespace world

namespace ai {
// Module-local AI connection, distinct from the engine's g_aiConnection. [UNRESOLVED] Original declaration file.
extern IBase*                aiConnection;
} // namespace ai


extern "C" bool initDll(ClassManager* pClsMgr)
{
    // Requires a non-null host ClassManager; this is an assertion, not a recoverable refusal.
    // The original assert attests Core/DllModule.cpp.
    assert(pClsMgr);

    classManager = pClsMgr;

    // The nineteen pulls, in the binary's order
    // The order is not alphabetical and not grouped by namespace; it is reproduced exactly because
    // nothing here proves it is arbitrary, and a reordering would be invisible to any check this
    // tree currently runs.
    settings              = reinterpret_cast<SettingsRepostitory*> (classManager->getSingleton("SettingsRepostitory"));
    mainConsoleObjects    = reinterpret_cast<ConsoleObjects*>      (classManager->getSingleton("MainConsoleObjects"));
    io::mainConsole       = reinterpret_cast<io::OldConsole*>      (classManager->getSingleton("MainConsole"));
    io::fileManager       = reinterpret_cast<io::FileManager*>     (classManager->getSingleton("FileManager"));
    io::fileChangeManager = reinterpret_cast<io::FileChangeManager*>(classManager->getSingleton("FileChangeManager"));
    bfGlobal_callback     = reinterpret_cast<DebugCallback*>(classManager->getSingleton("BfGlobal_callback"));
    bfGlobal_callbackData = reinterpret_cast<void**>       (classManager->getSingleton("BfGlobal_callbackData"));
    debugOn               = reinterpret_cast<bool*>        (classManager->getSingleton("DebugOn"));
    debugModules          = reinterpret_cast<Hash<std::string, std::string, StringHashFunc>*>(
                                                        classManager->getSingleton("DebugModules"));
    profilerClient        = reinterpret_cast<IProfilerClient*>     (classManager->getSingleton("Profiler"));
    g_system              = reinterpret_cast<System*>              (classManager->getSingleton("System"));
    loadStat              = reinterpret_cast<LoadStat*>            (classManager->getSingleton("LoadStat"));
    eventManager          = reinterpret_cast<EventManager*>        (classManager->getSingleton("EventManager"));
    frameEventManager     = reinterpret_cast<EventManager*>        (classManager->getSingleton("FrameEventManager"));

    world::objectManager         = reinterpret_cast<world::ObjectManager*>        (classManager->getSingleton("ObjectManager"));
    world::objectTemplateManager = reinterpret_cast<world::ObjectTemplateManager*>(classManager->getSingleton("ObjectTemplateManager"));
    world::playerManager         = reinterpret_cast<world::PlayerManager*>        (classManager->getSingleton("PlayerManager"));

    game              = reinterpret_cast<IGame*>(classManager->getSingleton("Game"));
    ai::aiConnection  = classManager->getSingleton("AIConnection");

    // The sixteen that are required
    // LoadStat, Game and AIConnection are fetched above and deliberately absent here. See the file
    // header. The chain's own order is the binary's: eventManager and frameEventManager are tested
    // last, after the three world managers, even though they were fetched before them.
    const bool ok =
           settings              != 0
        && mainConsoleObjects    != 0
        && io::mainConsole       != 0
        && io::fileManager       != 0
        && io::fileChangeManager != 0
        && bfGlobal_callback     != 0
        && bfGlobal_callbackData != 0
        && debugOn               != 0
        && debugModules          != 0
        && profilerClient        != 0
        && g_system              != 0
        && world::objectManager         != 0
        && world::objectTemplateManager != 0
        && world::playerManager         != 0
        && eventManager          != 0
        && frameEventManager     != 0;

    // Pass one: every module joins the host's registry
    for (int i = 0; dllmodules[i] != 0; ++i)
        classManager->registerServer(dllmodules[i]);

    // Pass two: only now is any module started
    // init()'s bool result is discarded. See the file header.
    for (int i = 0; dllmodules[i] != 0; ++i)
        dllmodules[i]->init();

    return ok;
}


extern "C" void deinitDll()
{
    // reverse order, and it is two passes again - the mirror image of initDll. Every module is
    // closed before any module is unregistered, so a module's close() can still reach its siblings
    // through the registry. The same reasoning as the two passes on the way in, running backwards.
    for (int i = getNoModules() - 1; i >= 0; --i)
        dllmodules[i]->close();

    for (int i = getNoModules() - 1; i >= 0; --i)
    {
        classManager->unregisterServer(dllmodules[i]);
        dllmodules[i]->release();
        // Clearing the table entry leaves the cached module count unchanged.
        dllmodules[i] = 0;
    }

    // neither `classManager` NOR the nineteen globals are cleared. Every one of them keeps
    // pointing into the host after the module is torn down. Harmless only because the image is
    // about to be unmapped.
}


int getNoModules()
{
    // Cache the nonzero module count. The static table must remain stable after initialization.
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

} // namespace hfe
} // namespace dice
