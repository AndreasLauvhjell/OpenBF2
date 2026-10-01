# Build tools

These are maintained build scripts and diagnostic harnesses. Start with the
[build guide](../../docs/building.md) for the CMake server, client and editor targets.
Regression fixtures live in [Code/Tests](../Tests).

| Entry point | Purpose |
| --- | --- |
| `python/build_python.cmd [x86|x64]` | Build and check the Windows embedded Python library. |
| `build.cmd` | Compile the engine and produce diagnostic link reports. |
| `ship.cmd` | Build an optimized dedicated-server package. |
| `renddx9.cmd`, `swiffplayer.cmd`, `bf2audio.cmd`, `textureatlasbuilder.cmd` | Build client plugins; `native` selects the rebuilt host ABI. |
| `aidll.cmd` | Build the separate AI image. |
| `_bmcheck.cmd`, `bootrun.cmd` | Compile or run the bounded boot harness. |
| `client_startup_trace.cmd` | Capture client startup diagnostics. |
| `frontier_sweep.py` | Find declarations that may lack callable implementations. |

`bf2audio_diff/` and `textureatlas_diff/` contain comparison harnesses and input
generators. `bf2audio_smoke/` and `demo/` contain focused launch checks.
`pyfixture/` supplies a minimal game-mode fixture for diagnostic runs; it is not
production game content.

The scripts keep their default object and binary outputs in ignored `obj*` and
`bin*` directories here. Python uses its architecture-specific output directory
under `Code/BF2/External/python-embedded/OBJ`. Use the documented output overrides
for concurrent builds, and keep rebuilt binaries separate from the retail game.

The boot harness and links made with `/FORCE` are diagnostic tools. Their success
establishes only the paths they exercised; use the production CMake run targets
for production startup checks.
