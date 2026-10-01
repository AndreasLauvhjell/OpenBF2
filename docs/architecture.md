# Engine architecture

## Images and boundaries

The repository contains several images built from overlapping source sets. The root
`CMakeLists.txt` defines the engine and client manifests; the directory tree alone does
not determine what links into an image.

`DEDICATED=1` selects the dedicated engine. `DEDICATED=0` selects the client and client
DLL code. Every engine compilation force-includes `Core/BuildConfig.h`, which rejects a
missing image define. A runtime `isDedicated()` check still matters in the client image:
the retail client supports a dedicated mode, so a runtime branch cannot automatically be
replaced by conditional compilation.

The client uses separate renderer (`RendDX9`), sound (`BF2Audio`), movie (`SwiffPlayer`),
and Python (`dice_py`) components. Plugin builds may target either the retail ABI or the
rebuilt host's native ABI. Match the plugin flavor to the host; matching filenames alone
does not make their C++ layouts compatible. The AI side-DLL has its own manifest and build target.

## Source map

All engine paths below are relative to [Code/BF2](../Code/BF2/).

| Area | Responsibilities and starting points |
| --- | --- |
| `Game/Main/` | Process startup, `BF2`, `BF2Engine`, module assembly, and the main loop. |
| `Core/` | Class and singleton registration, interfaces, events, settings, platform services, and diagnostics. |
| `IO/Console/`, `IO/File/` | Console commands and scripts, filesystem access, archive mounting, and resource lookup. |
| `IO/Network/` | Sockets, packets, connection transport, and networkable descriptions. `ProtocolLayer/` holds `NetServer` and `SWConnection`. |
| `Game/GameServer/`, `Game/GameClient/` | Session ownership, connection handling, loading, and server/client update paths. |
| `Game/GameLogic/`, `Game/Python/` | Level loading, game rules, and the embedded Python interface. |
| `Game/Objects/`, `Scene/` | Object templates, live world objects, components, and world registration. |
| `Game/GameEvents/`, `Game/Common/` | Serialized game events, shared game state, and ghost replication. |
| `Physics/`, `Math/`, `Animation/` | Simulation, collision, transforms, skeletons, and animation. |
| `Geom/`, `RendDX9/`, `RendMisc/` | Geometry resources, terrain, rendering, and renderer support. |
| `Menu/`, `Sound/` | Client interface and audio integration. |
| `External/` | Upstream dependencies and their local integration notes. |

[Code/Tools/BF2Editor](../Code/Tools/BF2Editor/) contains the editor application.
[Code/Tests](../Code/Tests/) contains focused fixtures and compile checks;
[Code/Build](../Code/Build/) contains build helpers and diagnostic harnesses.
These harnesses are scaffolding, not alternate production engine implementations.

## From startup to a running server

The production entry in `Game/Main/BF2.cpp` creates `BF2`, calls `init`, runs `BF2::run`
until it returns false, then destroys the application. `BF2Engine` owns engine startup,
command-line settings, the main loop, and shutdown. `Core/Module` and `ClassManager`
provide registered factories and named singletons used during assembly.

For a dedicated session, `BF2::init` reaches `BF2Engine::startGame`, which assembles the
game and invokes `GameServer::startServer`. The server constructs and initializes its
`NetServer` before selecting and validating a map. A no-bot session still opens a real
network listener.

The selected map reaches `GameServer::loadPath`, then `GameLogic::load`. Loading mounts
content, executes the level's console scripts, initializes game-mode Python, and publishes
world state through the engine's event and object systems. The order is observable: script
commands refer to registered types and resources, while synchronous load events can invoke
other subsystems before loading returns.

`BF2::run` delegates each iteration to `BF2Engine::mainLoop`. Timing supplies the server's
simulation step count and timestep; `GameServer::update` coordinates connection work,
simulation, events, and outgoing state. Loading also pumps a special update path, so seeing
an update call alone does not prove that normal simulation ran.

Shutdown includes game unloading, owner and reference release, listener shutdown, and
receive-thread completion. A successful startup test therefore also checks clean shutdown,
immediate port reuse, and reload. Record the configuration and paths exercised with every
test result; the presence of these methods does not establish runtime coverage.

## Content and the console

`.con` files are executable engine configuration. The console binds command names such as
`ObjectTemplate.*`, `Object.*`, and `sv.*` to typed engine operations. Templates describe
shared object definitions; instantiated world objects carry live state. Registration and
load order determine which names and resources a script can use.

`FileManager` resolves loose paths and mounted archives. The working directory, mod path,
archive order, and level directory all affect lookup. Diagnose a missing command or resource
at its registration or loading boundary before adding fallback behavior. Python game modes
use the embedded interpreter and native bindings; a fixture that bypasses those scripts
does not validate production content loading.

## The editor

The editor targets this engine's 1.50 content contract. Its application, UI, command history,
and editing modules are new code. It shares engine code with the client and isolates
editor-specific changes in shared translation units with `BF2_IMAGE_EDITOR`.

The editor's `App/` layer owns the window, engine host, editing session, and renderer
integration. `Framework/` supplies command history, selection, settings, file readers and
writers, and source-control interfaces. Editing tools live with the application. Its framework
checks run without starting the engine, while viewport and content tests need a game
installation and the rebuilt editor renderer.

The editor uses Dear ImGui and ImGuizmo for its interface. Its renderer and texture atlas
DLLs are built separately from the client's plugin outputs. The source-control interface
registers `NullSourceControl` by default; editing does not require Perforce. Console
bindings expose editing operations through the same session used by the UI; see the
[editor console](editor-console.md) for commands and replay behavior.

Retail-compatible files and `.con` semantics are the editor's compatibility boundary.
Compare output with retail-editor golden files. An unfinished operation reports its failure;
it must not claim to have saved or compiled content. Follow the [editor build steps](building.md#editor)
for its dependencies, checks, and output directories.
