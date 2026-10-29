# Artemis.Engine.Capture

Group `5_Engine` - static library inside the game process - copies one tick of the game into a Snapshot.

## Responsibility
The tick hooks (`ExecuteGameFrameDetour`, `SimulationTicksDetour`) that open the read window of every logic tick and wake the AI
thread; the locators of the object, player and interaction tables; and `CaptureService`, which copies the live objects and every
memory region of `RegionId` into a new Snapshot and publishes it.

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
The table bases and the engine thread context found by the locators; the last published Snapshot.

## Threads
The tick hooks run on the game thread and wait for the read of the tick (the window of `Engine.Base`). `Capture` runs on the AI
thread inside that window; the game goes on as soon as the AI thread ends the read.

## Errors
A region that cannot be read is left empty in the Snapshot and its flag stays off; before the thread context or the object table
is found, the Snapshot says so and nothing is copied.

## Requirements
The read window stays short: the game waits for it on every tick (telemetry: read wait).

## Tests
`ReplayTests.Recordings` uses Snapshots this project recorded; the capture itself is checked in game (golden film and the bench).
