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
and Python (`dice_py`) components. Rebuilt renderer, sound, and movie plugins target the
rebuilt host's native ABI and share its dynamic CRT and release STL layout. Mixing these
plugins with the retail executable, or retail engine plugins with the rebuilt executable,
is unsupported. The AI side-DLL has its own manifest and build target.

This is an intentional policy change under [issue #7](https://github.com/B2BF/OpenBF2/issues/7),
`[CHOSEN, not recovered]`. Gameplay compatibility with retail clients and existing content
remains a requirement. The first removal pass retires compatibility plugin builds and
the `Memory.dll`/MSVCR71 heap adapters, and stages a rebuilt TextureAtlasBuilder with the
client. TextureAtlasBuilder retains its static CRT and legacy float formatting because
those choices preserve `.tai` output, independently of executable compatibility.

The cleanup is staged. Renderer-internal `vc71::*` containers, fixed-offset consumers,
factory calling-convention thunks, and legacy declarations used by shared code and reference
fixtures remain until their callers can be migrated together. `BF2_PLUGIN_ABI_NATIVE=0`
is rejected for renderer, movie, and audio plugin images; its remaining uses do not provide
a supported retail plugin build. Dedicated/non-x86 alternatives and unresolved native
frontiers retain their existing behavior.

SwiffPlayer's wrapper now contains only its native host paths; the retail-host conditional
arms have been removed under the same issue. The native ActionScript callback whitelist and
the `swiff-native-as-host` and `swiff-host-bodies` frontiers remain, along with the RW-88
callback/tracing, RW-89 crash-tracing, and RW-49 module-consolidation boundaries. The documented
`voipStopTest` report-and-continue exception is preserved. Its internal `vc71::*` strings, player-stat
records, and queued-mouse storage still have live consumers and remain for a later migration.
This removal does not change movie formats, callback behavior, or host interface layouts.

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

## Client menu disconnect

The stock `mainMenu.swf` disconnect page queues `dice.bf2.Logic.disconnectGame`
through its UpdateManager, then polls `Logic.isGameRunning` while displaying the
disconnect modal. A missing native callback leaves the game running and the page
waiting indefinitely. `SwiffPlayer.cpp` registers the callback in the rebuilt plugin.
It requests shutdown on the player's host while the separately looked-up registry
host owns the busy cursor. The registry host may be absent; the player and its host
are required. Arguments are ignored and the ActionScript result is left untouched.

This bridge follows the original x86 BF2 1.50 `SwiffPlayer.dll` (MD5
`b4ef39939007a555f23829824c021e35`): registration at `0x1004DFC3` binds the
callback at `0x1004CB10`; `0x10013520` establishes the cursor/busy scope. The callback
invokes host slot `+0xB4`. In BF2.exe, `SwiffHost::requestShutdownGame` at `0x491860`
queues event 17, and `BF2Engine::renderMenu` at `0x40BD10` consumes it through
`shutdownGame(false)`. `SwiffHost::isGameRunning` at `0x486C90` reports whether
the global game still exists. The movie clears its modal after that becomes false.
The movie library is client-only; there is no dedicated-server ELF counterpart to
this callback. No host layout, event ID, or network protocol changes are needed.

`Code/Tests/swiff_disconnect_runtime.cmd` resolves the registered ActionScript
member instead of calling `SwiffHost::requestShutdownGame` directly. It takes a
fresh native plugin object directory and an isolated test output directory. The optional
`native` argument remains accepted. Distinct registry and player hosts expose
incorrect cursor ownership; the callback must preserve call order and the
caller's result even when the host returns false. This boundary check does
not establish joined-client teardown or reconnect stability. The existing
`swiff-native-as-host`, `swiff-host-bodies`, and `RW-88` frontiers outside this
callback remain subject to their existing recovery and validation requirements.

## Object-owned sounds

Weapon components request `Fire1P`, `Fire1P_Outdoor`, `Fire3P`, and their other
samples through `SimpleObjectTemplate::createSoundFromTemplate`. The client factory
looks up `S_<object-template-name>_<sound-name>` with sound class ID 9986001,
requires an engine template and a nonempty filename, then passes that template and
the owning object to `BfSound::createSound`. That facade assigns the sound's owner
and obstruction evaluator. The same factory serves vehicle, projectile, soldier,
and ambient object sounds.

The original BF2 1.50 x86 BF2.exe client implementation is `0x624A20` (image MD5
`4c843dcee79388341f72c3a7adb1ace6`); weapon initialization at `0x608920` calls it for
all eleven weapon sound slots. Its four `impl()` calls and
two filename reads are preserved, as are the runtime dedicated-mode and missing
sound-facade guards. The symbolized dedicated ELF implementation at `0x081F2760`
returns null without looking anything up. The shared source now selects that
server implementation only for `DEDICATED=1`. Previously, the server body was also
used in the client, leaving every weapon sound slot empty before firing reached
the audio engine. This recovery changes no interface layout or sound-file format.

`Code/Tests/weapon_sound_runtime.cmd <isolated-output-directory> client` checks the
factory guards, owner forwarding, all eleven weapon sound slots, single and
automatic third-person firing, stopping, and sound release. Run it again with a
different output directory and `dedicated` to check the server's null factory.
The fixture copies the selected production bodies verbatim under their real
headers and compiles the actual weapon sound component; its surrounding observers
provide no audio device or game world. `BF2_TEST_FACTORY_SOURCE` can select a
pre-fix source snapshot to verify that the client regression fails before the fix.

Factory and weapon-component checks establish sound creation and playback dispatch;
they do not establish audible output from a live joined client. Device playback,
first/third-person mixing, and repeated gameplay still need runtime validation.
Existing `bfsound-debug-draw` frontiers and the opt-out `BF2AUDIO_NO_VORBIS` decoder
frontiers are outside this factory and remain unchanged.

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
