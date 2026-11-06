# Artemis.Engine.Capture

Group `5_Engine` - static library inside the game process - copies one tick of the game into a Snapshot.

## Responsibility
The tick hooks (`ExecuteGameFrameDetour`, `SimulationTicksDetour`) that open the read window of every logic tick and wake the AI
thread (`SimulationTicksDetour` also ends the round on request and hands a pending gametype preset to `Engine.Base` `PresetService` with
the live game variant at a round break); the locators of the object, player and interaction tables; and `CaptureService`, which copies the live objects (the slots below the first-unused mark of the engine object table, at most its capacity and `k_MaxObjects`, and tells `Services` Telemetry the mark and live count) and every
memory region of `RegionId` into a new Snapshot and publishes it. `CaptureLight` (Q101) copies only what the flag pass needs, into a light Snapshot (`IsLight`) that it does not publish: the objects of kind `k_LightKindId` (the weapons: the flags are weapons, and this project cannot filter by tag) and the regions of `IsLightRegion` (game clock, game engine globals, megalo objects). It reads the same table with the same walk, takes no telemetry and does not check the capacities.

## Does not
- Interpret what it copies: that is `Blam.Runtime`.
- Apply actions or end rounds by itself (`Engine.Control`, `Mod`).

## Input and output
- Input: the game memory and the layouts of `Blam.Layout`.
- Output: the published `Snapshot` in `CaptureStore`, and the tick events of the read window.

## Public interface
Other projects may import only these modules; everything else in the project is internal.

| Module | What it gives |
|---|---|
| `Engine.Capture.Layer` | The facade: `CaptureStore`, `CaptureService`, the three locators and the two tick detours. Owner only. |
| `Engine.Capture.State` | `CaptureStore`: the addresses the capture copies from and the last published Snapshot (for the dev tools). |

## Depends on
`Common`, `Services`, `Engine.Base`, `Blam.Layout`.

## State between ticks
The table bases and the engine thread context found by the locators; the last published Snapshot. `CaptureService` also keeps a pool of up to 4 Snapshots it reuses (D276): `Capture` fills one that only the pool holds (never the published one, nor one a reader, the dump queue or a viewer still holds), emptying its objects and reusing the capacity of its arena and regions. A Snapshot nobody can release because it is held is simply not reused, and above the pool capacity a new one is made and not kept. A new teardown epoch starts a new pool. `CaptureLight` takes its Snapshot from the same pool and empties the regions that are not light, so a reused Snapshot never carries a full capture's regions.

## Threads
The tick hooks run on the game thread and wait for the read of the tick (the window of `Engine.Base`). `Capture` runs on the AI
thread inside that window; the game goes on as soon as the AI thread ends the read.
The pool belongs to `Capture` and the AI thread alone: `Cleanup` (game thread) only drops the published Snapshot of `CaptureStore` and never touches the pool, which a new epoch replaces on the AI thread.

## Errors
Once per map load (per teardown epoch of `LifecycleStore`), `Capture` compares the capacity of the engine object and player tables (header of the data array) with `k_MaxObjects` and `k_MaxPlayers`: a difference is a WARNING with both values (a larger engine table is walked only up to `k_MaxObjects`). A region that cannot be read is left empty in the Snapshot and its flag stays off; before the thread context or the object table
is found, the Snapshot says so and nothing is copied.

## Requirements
The read window stays short: the game waits for it on every tick (telemetry: read wait).

## Tests
`ReplayTests.Recordings` uses Snapshots this project recorded; `CaptureServiceTests` (the capacity check over fake memory, and `CaptureLight` equal to `MakeLight` of the full capture, not published, sharing the pool); the capture itself is checked in game (golden film and the bench).
