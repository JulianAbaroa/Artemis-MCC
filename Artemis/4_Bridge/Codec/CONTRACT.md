# Artemis.Bridge.Codec

Group `4_Bridge` - pure static library - the order of the tick, the Tick, and its flat and recorded forms.

## Responsibility
- `PipelineService::Run`: the one place where the order of a tick lives (Blam.Runtime, then Perception, then Views, then the
  assembly of the Tick), over a Snapshot. The AI thread and the tests call it.
- The Tick: everything the layers built, packed into one immutable value per tick.
- Flattening: the Tick into the `BridgeTick` of `Contract` (players, entities, flags, stands, match with round, team scores, round time, round limit and score to win,
  newest events as POD rows). Copies, never computes. The one thing it does is order the flags and the stands, which Perception
  keeps by handle, by `BaseTeam` and handle, so a flag keeps its row while the engine recreates it.
- The `.tdump` format (the recorded stream of flattened ticks) and its writer. Version 2 writes the flags and the stands (counts in
  the `BridgeTickHeader`) after the events of every record; a reader of version 1 files finds none.

## Does not
- Open shared memory or talk to the client: that is `Bridge.Transport`.
- Read the game, time the stages or create threads: `Mod` passes a callback that times each stage.
- Decode the actions of the client: `Control.Lockstep.System` turns them into overrides.

## Input and output
- Input: a Snapshot and its generation; the Layers of `Blam.Runtime`, `Perception` and `Views`.
- Output: the Tick in `TickStore`; on decision ticks, a `FlatRequest` handed to the bridge thread; `.tdump` records.

## Public interface
Other projects may import only these modules; everything else in the project is internal.

| Module | What it gives |
|---|---|
| `Bridge.Codec.Layer` | The facade: `TickService`, `FlatService`, `PipelineService` and their stores; `Cleanup()`. |
| `Bridge.Codec.Pipeline.System` | `PipelineService::Run` and the names of its stages. |
| `Bridge.Codec.Pipeline.Type` | `ObservedTick`: what the AI thread hands to the tick observers. |
| `Bridge.Codec.Tick.Type`, `Bridge.Codec.Tick.State` | The Tick and its store. |
| `Bridge.Codec.Flat.Type`, `Bridge.Codec.Flat.System` | `BridgeClock`, `FlatRequest`, `FlatService::Flatten`. |
| `Bridge.Codec.Dump.Type`, `Bridge.Codec.Dump.System` | The `.tdump` format and writer. |

## Depends on
`Common`, `Contract`, `Blam.Layout`, `Blam.Map`, `Blam.MapModel`, `Blam.Runtime`, `Perception`, `Views`.

## State between ticks
The last Tick and one pending `FlatRequest` (a newer one replaces a request the bridge thread has not taken; in Lockstep the game
waits, so it never happens there).

## Threads
`Run` and the assembly run on the AI thread. `Flatten` runs on the bridge thread (and the dump thread for recordings). The handoff
is the one-slot `FlatStore`.

## Errors
More rows than the `BridgeTick` holds are cut and counted in its header (`EntitiesTruncated`, `ObjectivesTruncated`).

## Requirements
- The flattened layout changes only through `Contract` (same `k_BridgeLayoutHash`).
- Deterministic: a recorded Snapshot stream gives the same `.tdump` byte for byte; padding of every row is zeroed.

## Tests
`PipelineTests` (the pure chain without the DLL), `ReplayTests.Recordings` (`.sdump` in, golden `.tdump` out, byte for byte),
`FlatTests` (flags, stands, round, team scores and both event sequences), `TickDumpWriterTests` and `CollidableSceneTests` (the pipeline builds no scene), in `Artemis.Tests`.
