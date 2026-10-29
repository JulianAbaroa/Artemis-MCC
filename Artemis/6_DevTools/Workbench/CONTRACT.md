# Artemis.DevTools.Workbench

Group `6_DevTools` - static library inside the game process - the launcher, inspectors, recorders and the in-game test bench.

## Responsibility
- The launcher and its tab windows: simulation (bots, speed, pause), capture, object and player tables, definitions inspector,
  logs, settings and hotkeys (icons from `Resources/`, stb compiled here).
- Recorders: TickDump (`.tdump` + `.sdump` of every tick, written by the dump thread) and TickTrace (engine clock and objects per
  tick).
- The in-game test bench: takes scenarios from the exchange folder of `Tools/game.py`, plays their matches through the match cycle,
  checks them and writes `state.json` and the report.

## Does not
- Take part in the tick: the AI thread only hands it the immutable `ObservedTick`.
- Import the engine Layers: it uses `EngineAccess` from `DevTools.Viewer`.

## Input and output
- Input: `ObservedTick` per tick, `EngineAccess`, user actions, bench scenarios.
- Output: windows on the game frame, `.tdump`/`.sdump`/trace files, bench reports, preferences.

## Public interface
Other projects may import only these modules; everything else in the project is internal.

| Module | What it gives |
|---|---|
| `DevTools.Workbench.Layer` | The facade: stores and services of the launcher and its tabs. |
| `DevTools.Workbench.TickDump.System` | The TickDump service the dump thread runs. |
| `DevTools.Workbench.Bench.System` | The bench service the bench thread runs. |

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
The AI thread never waits for a tool except the bounded queue of the dump.

## Tests
The formats it writes are tested where they live (`TickDumpWriterTests` for `Bridge.Codec`, `SnapshotRecordTests` for
`Blam.Layout`); `Tools/Tests/test_tickdump.py` covers the reader. The windows are manual; the bench is itself the in-game test
harness.
