# Artemis.Bridge.Codec

Group `4_Bridge` - pure static library - the order of the tick, the Tick, and its flat and recorded forms.

## Responsibility
- `PipelineService::Run`: the one place where the order of a tick lives (Blam.Runtime, then Perception, then Views, then the
  assembly of the Tick), over a Snapshot. The AI thread and the tests call it.
- The Tick: everything the layers built, packed into one immutable value per tick. It carries the `MapId` of the scnr of the loaded map (0 while none), the `PossessedPlayer` (handle of the bot the user possesses, 0xFFFFFFFF when none; VW.9c.2 B, D351; `TickService::SetPossessedSource` connects where it is read, an atomic; domain only: the flat `BridgeTick` and its layout hash do not carry it yet), the affordances of the local player and the Views of the agents (`Tick.Views`, one per row of `Players`, with the aim hit and the rays of each), which do not cross the bridge yet. The interaction of the Tick is the one of `k_PrimaryLocalUser`.
- Flattening: the Tick into the `BridgeTick` of `Contract` (players, entities, flags, stands, weapons, projectiles, explosives, lifts, match with round, team scores, round time, round limit and score to win,
  newest events as POD rows). Copies, never computes. The one thing it does is order the flags, the stands, the weapons, the projectiles, the explosives and the lifts, which Perception
  keeps by handle (the flags by `BaseTeam` and handle), so a flag keeps its row while the engine recreates it.
- The `.tdump` format (the recorded stream of flattened ticks) and its writer. Version 2 writes the flags and the stands (counts in
  the `BridgeTickHeader`) after the events of every record; a reader of version 1 files finds none. Version 3 adds the weapons and then the projectiles (counts in the header) after the stands; a
  reader of older files finds none. Version 4 adds the explosives (count in the header) after the projectiles. Version 5 adds the lifts after the explosives.
  The record header carries the real microseconds since the previous record (flag 8 when measured, 0 and off otherwise, so older files read as not measured and the version is the same). The writer takes it from the caller; a replay passes none.

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
| `Bridge.Codec.Pipeline.System` | `PipelineService::Run` (it takes the `AgentSet` of the tick, every player by default) and the names of its stages (23, `Affordance` and `Views` last before `Assemble`), and `RunLight` (Q101): decodes a light Snapshot into the light stores of Blam.Runtime and runs the flag pass of Perception. It touches no store of the full tick and assembles no Tick. |
| `Bridge.Codec.Pipeline.Type` | `CapturedTick` and `TickQueue`: one tick the game thread copied and queued for the AI thread (Snapshot, bridge clock, generation, epoch, light, lost before, first after load), and the queue that carries them (D305). `ObservedTick`: what the AI thread hands to the tick observers. |
| `Bridge.Codec.Tick.Type`, `Bridge.Codec.Tick.State` | The Tick and its store. |
| `Bridge.Codec.Flat.Type`, `Bridge.Codec.Flat.System` | `BridgeClock` (with `IsDecision` and `IsSparse`), `FlatRequest`, `FlatService::Flatten`. |
| `Bridge.Codec.Dump.Type`, `Bridge.Codec.Dump.System` | The `.tdump` format and writer. |

## Depends on
`Common`, `Contract`, `Blam.Layout`, `Blam.Map`, `Blam.MapModel`, `Blam.Runtime`, `Perception`, `Views`.

## State between ticks
The last Tick and one pending `FlatRequest` (a newer one replaces a request the bridge thread has not taken; in Lockstep the game
waits, so it never happens there).

## Threads
`Run` and the assembly run on the AI thread. `Run` asks its `canPublish` right before assembling the Tick and leaves the last Tick as it was when it says no (the engine started tearing down). `Flatten` runs on the bridge thread (and the dump thread for recordings). The handoff
is the one-slot `FlatStore`.

## Errors
More rows than the `BridgeTick` holds are cut and counted in its header (`EntitiesTruncated`, `ObjectivesTruncated`, `WeaponsTruncated`, `ProjectilesTruncated`, `ExplosivesTruncated`, `LiftsTruncated`).

## Requirements
- The flattened layout changes only through `Contract` (same `k_BridgeLayoutHash`).
- Deterministic: a recorded Snapshot stream gives the same `.tdump` byte for byte (the replay does not measure real time); padding of every row is zeroed.

## Tests
`PipelineTests` (the pure chain without the DLL), `ReplayTests.Recordings` (`.sdump` in, golden `.tdump` out, byte for byte),
`FlatTests` (flags, stands, round, team scores, the `MapId` of the header, the `RespawnTicks`, the traits, the custom powerup of each player, the variant settings of the match, ground and crouch of the players, the lifts with their stages and exit and both event sequences), `TickDumpWriterTests` and `CollidableSceneTests` (the pipeline builds no scene), in `Artemis.Tests`.
