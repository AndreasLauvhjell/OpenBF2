# Staged boot-harness binaries

This is the default binary output directory for
[bootrun.cmd](../bootrun.cmd). The script links `bf2_w32ded.exe` from an existing
engine object manifest and places `dice_py.dll` beside it.

Build CPython with [build_python.cmd](../python/build_python.cmd), then build the
engine before running the boot harness. Object files, import libraries, debug
symbols, maps, and logs belong in the object output directory selected by
`BF2_BUILD_OUT` (default: `Code/Build/obj`).

The harness runs with the game-content directory as its working directory so
`mods/` and `python/` resolve. Keep rebuilt binaries separate from the original
game installation. This directory contains staged build output, not game assets.

## Python and runtime linkage

The Windows harness uses CPython 2.3.4 as `dice_py.dll`. Keep that DLL beside the
executable and use a matching architecture. The harness, engine, and Python build
use the shared MSVC runtime (`/MD`).

Leave `Py_NO_ENABLE_SHARED` undefined for this DLL configuration. CPython's
consumer headers request `python23.lib`; these build scripts suppress that
default with `/NODEFAULTLIB:python23.lib` and explicitly link `dice_py.lib`.
The matching Visual C++ runtime must be installed.

`bootrun.cmd` uses a diagnostic `/FORCE` link. A run establishes only the paths
it exercised; producing an executable does not establish a complete production
link.
