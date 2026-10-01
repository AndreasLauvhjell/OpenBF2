# Vehicle regression harness

These test observers use an isolated x86 engine-object manifest and the
`BF2::init -> BF2Engine::startGame -> GameServer` path with stock map loading and
non-coop `gpm_cq/64`. They are excluded from production build manifests.

## Run

From the repository root:

```powershell
$vehicleBuild = Join-Path $PWD 'Code/Build/obj_vehicle_regression'
$vehicleRuns = Join-Path $PWD 'Code/Build/vehicle_regression'
$env:BF2_BUILD_OUT = $vehicleBuild
cmd /c Code\Build\build.cmd
Remove-Item Env:BF2_BUILD_OUT
python Code/Tests/tier2/run.py --build-out $vehicleBuild --out $vehicleRuns
```

Use separate engine-build and test-output directories. The engine manifest must
come from a completed compile; inspect compile failures before using it.
Stock game content and `dice_py.dll` must be available. Override `--game-dir`
and `--vcvars` for your installation. The runner uses Python's standard library.

Use repeatable `--case` options to select tests, such as `--case lav_tow`.
Keep `aircraft.tsv` and `floating.tsv` beside the runner: they are its expected
values, not generated reports.

| Cases | Coverage |
| --- | --- |
| `bodies` | Object methods and properties |
| `aircraft` | Stock properties, regulators, wheel bounds, and parked aircraft |
| `floating` | Sea level, buoyancy, throttle, and steering |
| `z8_float`, `uh60_float`, `mi17_float` | Synthetic-player entry, finite physics, and vehicle serialization |
| `lav_tow` | Mapped-input launches, finite physics, and projectile/vehicle serialization |
| `parachute_water`, `parachute_land` | Exit, deployment, attachment, camera input, and landing |
| `moving_exit` | Ground-vehicle exit and retained health |
| `su34_exit`, `su34_copilot` | Exit momentum and copilot firing |
| `collision` | Damage arithmetic, contact suppression, penetration, and vehicle contact |
| `commander` | Supply flight, landing, serialization, and artillery |
| `sidewinder`, `archer`, `friendly` | Hostile tracking and friendly rejection |

## Isolation and results

Each child receives explicit test inputs in a cleared `BF2_*` environment.
The generated configuration disables AI and external services and sets both
`sv.maxPlayers 64` and `+maxPlayers 64` so the intended map layer is selected.

Default game/query endpoints are `127.0.0.1:16986` and `127.0.0.2:29886`.
The runner checks availability, verifies loopback sockets, and refuses port
16567. Override `--port` and `--query-port` with unused ports. Successful cases
must complete shutdown and immediately rebind the game port.

`results.json` records exit status, frames, ports, inputs, and log paths.
The runner returns nonzero if any selected case fails. Instrumentation is
inserted into private source copies; missing or ambiguous anchors fail the build,
and a failed link cannot reuse a stale executable.

The runner uses a diagnostic `/FORCE` link. Passing cases cover only their
observed paths. Synthetic players and isolated serialization do not establish
retail-client connectivity, client presentation, reconnect, map reload, or
long-running occupied-server stability.
