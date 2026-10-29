# Artemis.Engine.Match

Group `5_Engine` - static library inside the game process - the cycle of matches and their recorded configurations.

## Responsibility
Sees and drives the match cycle of MCC: copies the vtable of the engine interface (`EngineInterfaceDetour`) to know when an engine
starts and is destroyed, keeps a library of recorded match configurations (`<storage>/MatchConfigs/<name>.bin`) and transplants
the pending one into the next start, and presses START on the custom games lobby from the MCC lifecycle update
(`LifecycleUpdateDetour`).

## Does not
- Decide which match comes next: the bench or the Python client ask through `Mod`.
- Read the state of the tick.

## Input and output
- Input: a request to launch a recorded configuration, or to leave.
- Output: a launched match; the cycle state (lobby open, engines started and destroyed).

## Public interface
Other projects may import only these modules; everything else in the project is internal.

| Module | What it gives |
|---|---|
| `Engine.Match.Layer` | The facade: config and cycle stores, `MatchConfigService` and the two detours. Owner only. |
| `Engine.Match.Type` | Engine interface vtable entries and the `LoadGameEngine` signature. |
| `Engine.Match.State` | `MatchConfigStore` and `MatchCycleStore`. |
| `Engine.Match.System` | `MatchConfigService`: list, record, load and transplant configurations. |

## Depends on
`Common`, `Services`, `Engine.Base`.

## State between ticks
The recorded configurations on disk (they hold the user's XUID: never in the repo) and the pending one; the counts of the cycle.

## Threads
The detours run on the MCC thread that updates the lifecycle. Stores are thread-safe for the bench and the UI. Listing the folder
happens on a user action, never every frame.

## Errors
An incompatible configuration or the wrong screen does not launch; the reason is logged and visible in the cycle state.

## Requirements
The format of the recorded configurations stays readable.

## Tests
`MatchConfigTests` (configurations on disk) in `Artemis.Tests`; the cycle in game with the bench's rotation scenarios.
