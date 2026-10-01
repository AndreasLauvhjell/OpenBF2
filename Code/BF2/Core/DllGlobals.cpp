// Host service pointers for side-DLL images; never link these definitions into the engine.
// initDll fills these zero-initialized pointers before any module's init() runs. The DLL borrows
// the host console, console registry and LoadStat; it must not construct duplicate owners.
// The module uses aiConnection, distinct from the engine's g_aiConnection.
// [INFERRED] filename; [CHOSEN] grouping. [UNRESOLVED] original globals filename and full membership.
// Pointer declarations match the engine interfaces used by the handshake.

namespace dice {
namespace hfe {

// The forward declarations are spelled here rather than pulled in through headers, which is the
// convention every consumer of Core/Globals.cpp follows - no header declares these globals, so each
// translation unit that touches one writes its own extern. Core/DllModule.cpp does exactly this.
class ClassManager;                         // Core/ClassManager.h; attested param type IClassManager*
class ConsoleObjects;                       // Core/ConsoleObjects.cpp - not an IBase
class EventManager;
class IBase;
class IGame;
class IProfilerClient;
class LoadStat;                             // Core/LoadStat.h
class SettingsRepostitory;                  // dice's spelling, frozen - see Core/Globals.cpp
class System;

namespace io    { class FileChangeManager; class FileManager; class OldConsole; }
namespace world { class ObjectManager; class ObjectTemplateManager; class PlayerManager; }

// The parameter initDll is handed
ClassManager*         classManager           = 0;

// The nineteen, in initDll's own order
SettingsRepostitory*  settings               = 0;   // libaidll .bss 0x4DD3EC
ConsoleObjects*       mainConsoleObjects     = 0;   // libaidll .bss 0x4DD3F4
// Debug globals are defined by Debug.cpp and rebound to host storage before module initialization.
// The original DLL's storage-free startup layout remains [UNRESOLVED].
IProfilerClient*      profilerClient         = 0;
System*               g_system               = 0;
LoadStat*             loadStat               = 0;   // libaidll .bss 0x4DD414
EventManager*         eventManager           = 0;
EventManager*         frameEventManager      = 0;
IGame*                game                   = 0;

namespace io {
OldConsole*           mainConsole            = 0;   // libaidll .bss 0x4DD428
FileManager*          fileManager            = 0;
FileChangeManager*    fileChangeManager      = 0;
} // namespace io

namespace world {
ObjectManager*         objectManager         = 0;
ObjectTemplateManager* objectTemplateManager = 0;
PlayerManager*         playerManager         = 0;
} // namespace world

namespace ai {
// The module uses aiConnection; the engine has a distinct g_aiConnection global.
IBase*                aiConnection           = 0;   // libaidll .bss 0x4DD44C
} // namespace ai

} // namespace hfe
} // namespace dice
