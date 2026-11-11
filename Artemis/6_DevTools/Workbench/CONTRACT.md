# Artemis.DevTools.Workbench

Group `6_DevTools` - static library inside the game process - the launcher, inspectors, recorders and the in-game test bench.

## Responsibility
- The launcher and its tab windows: simulation (bots, speed, pause), object and player tables, definitions inspector,
  logs, settings and hotkeys (icons from `Resources/`, stb compiled here).
- The raycast oracle (R3, D272, `Oracle`): checks in game that the rays of Artemis give what the engine gives and measures what a ray costs the engine. Off by default and never saved as on (only its rays per batch, default 16, and its period in ticks, default 1, are preferences: `RaycastOracle_Rays`, `RaycastOracle_EveryTicks`). Every period it hands `Engine.Capture` a batch with the rays of the last tick (aim and perception) and random ones from the eye of the local player (fixed seed, 100 m), reads what the game thread traced, compares it with `Raycaster::Cast` (static, same ray) and with the `RaycastService` result of the tick (one or two ticks apart), and every 10 s writes `[RaycastOracle] INFO:` to the log and `RaycastOracle/<date>.txt` in the storage folder: counters, the 20 worst differences and the engine cost per ray (mean, p50, p99) for the static test and the test with objects. The comparison is `Views.Oracle`. Exception to "observers must not block", only while the oracle is on: its observer writes the report file (a few KB of text, truncated and rewritten every 10 s) on the AI thread, and the rays it casts with `Raycaster` cost up to 64 casts per tick. Nothing in the repo posts work to the Dump thread (it only runs the TickDump queue), so the write stays there.
- Recorder: TickDump (`.tdump` + `.sdump` of every tick, written by the dump thread; each `.tdump` record also carries the real
  time since the previous one).
- The in-game test bench: takes scenarios from the exchange folder of `Tools/game.py`, plays their matches through the match cycle,
  checks them and writes `state.json` and the report. A match may ask for the TickDump (`tickdump` for the `.tdump`, `tickdump full`
  for the `.tdump` and the `.sdump`), which the bench starts at the first tick of the match and stops at its end.
  Every-tick demand (Q101): TickDump raises it while it records (from its start to its stop or its failure) and each bench match raises it
  for its whole duration, so the pipeline runs on every tick and the checks, the probes and the dump see every tick even if a client asked
  for sparse. A match with `every_tick off` does not raise it, so a client may run the sparse mode there (the bench then sees only the
  ticks of the decisions); it cannot go with `tickdump` or `events`. The `perf` block also has a `light` entry (ticks, mean and max in microseconds).
  Each match report also gets a `perf` block (omitted when the run had no sweeps): the cumulative telemetry counters and the Lockstep
  counters are photographed when the run begins and when it ends, and the difference gives ticks, wall, engine (without the Lockstep
  wait), read wait and sweep times (avg, p50, p99, max) per tick or sweep, the average of each sweep stage, and the Lockstep waits, plus the highest first-unused slot (`object_slots_max`) and live count (`objects_max`) of the engine object table.

## Does not
- Take part in the tick: the AI thread only hands it the immutable `ObservedTick`.
- Import the engine Layers: it uses `EngineAccess` from `DevTools.Viewer`.

## Input and output
- Input: `ObservedTick` per tick, `EngineAccess`, user actions, bench scenarios.
- Output: windows on the game frame, `.tdump`/`.sdump` files, bench reports, preferences.

## Public interface
Other projects may import only these modules; everything else in the project is internal.

| Module | What it gives |
|---|---|
| `DevTools.Workbench.Layer` | The facade: stores and services of the launcher and its tabs. |
| `DevTools.Workbench.TickDump.System` | The TickDump service the dump thread runs. |
| `DevTools.Workbench.Oracle.System` | The oracle service `Mod` runs as a tick observer (`OnTick`, AI thread: it never waits for the game thread) and draws in the Simulation tab. |
| `DevTools.Workbench.Bench.System` | The bench service the bench thread runs. A match may name a gametype preset (`preset <name>` or `preset none`, made active before the launch, a name that cannot be loaded ends the match with `no-preset`) and check `preset_applied [<name>]`. It may also name a map variant (`map <config name>` or `map none`, Q98, D266: the map variant of a recorded config, read and made active before the launch, a name that cannot be read ends the match with `no-map`) and check `map_applied [<name>]`. It names the bots of the match with a roster (`roster <8 counts> auto <n>`, applied by the bot service at the first tick of the match, a match without one has no bots: the previous roster is cleared) and one roster per next round (`round_roster ...`, LS.1, D284: asked for while the round before runs, applied at its end, and a `round_roster` without a `roster` is an error) and checks `roster_ok` (at the start of every round in play the bots per team of the Tick match the roster of that round). Its panel shows one line with the preset and one with the map variant and what the last load did with each. The Simulation tab shows the roster line (what is active and its state) and an editor of the counts per team that requests another one. |

## Depends on
Every pure project, `Services`, `Bridge.Transport`, the five `Engine.*` projects, `DevTools.Gui` and `DevTools.Viewer`.

## State between ticks
Window state and preferences, the recording queues and files, the running bench scenario.

## Threads
Render thread for the windows; the AI thread only pushes to the recorder queues (bounded); the dump thread writes the dumps; the
bench thread advances the bench five times per second.

## Errors
A tool that fails does not affect the tick; a bench match that fails or passes its limit is left and the next one starts.

## Requirements
The AI thread never waits for a tool except the queue of the dump, which is capped in bytes (256 MB of what the entries keep alive; an entry alone above the cap still goes in; no Tick is lost).

## Tests
The formats it writes are tested where they live (`TickDumpWriterTests` for `Bridge.Codec`, `SnapshotRecordTests` for
`Blam.Layout`); `Tools/Tests/test_tickdump.py` covers the reader. The windows are manual; the bench is itself the in-game test
harness.
