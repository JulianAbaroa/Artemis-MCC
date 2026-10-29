# Artemis.Blam.Runtime

Group `2_Blam` - pure static library - the live objects, players, interaction and game state of one tick.

## Responsibility
Interprets the Snapshot of a tick with the static model of the map: the alive objects (with bone matrices and damage sections),
the players of the player table, the interaction of every local player, and the game globals (clock, phase, scores) with the new
entries of the engine's event ring.

## Does not
- Read the process or follow pointers: `Engine.Capture` already copied everything into the Snapshot. No type holds a process
  address.
- Classify objects or build entities: that is `Perception`.

## Input and output
- Input: a `Blam::Layout::Snapshot::Type::Snapshot` and `Blam::MapModel::Layer`.
- Output: the stores of `Blam::Runtime::Layer` (`ObjectTableStore`, `BoneMatricesStore`, `DamageSectionsStore`, `PlayerTableStore`,
  `InteractionStore`, `GameStore`), one published value per tick.

## Public interface
Other projects may import only these modules; everything else in the project is internal.

| Module | What it gives |
|---|---|
| `Blam.Runtime.Layer` | The facade: stores and services; `Cleanup()` forgets the map. |
| `Blam.Runtime.Object.Type`, `Blam.Runtime.Object.State` | `AliveObject`, the object table, bone matrices, damage sections. |
| `Blam.Runtime.Player.Type`, `Blam.Runtime.Player.State` | `AlivePlayer` and the player table. |
| `Blam.Runtime.Interaction.Type`, `Blam.Runtime.Interaction.State` | Interaction of each local player. |
| `Blam.Runtime.Game.Type`, `Blam.Runtime.Game.State` | Game globals of the tick, event cursor and dropped events. |

## Depends on
`Common`, `Blam.Layout`, `Blam.Map`, `Blam.MapModel`.

## State between ticks
- The objects and players are kept by handle and refreshed every tick; a handle whose salt changed is replaced.
- `GameStore` keeps the cursor of the event ring (last sequence read) and the count of entries overwritten before they were read.

## Threads
Updates run on the AI thread, called by `Bridge.Codec.Pipeline.System`. Readers on other threads use the published tables and
never wait.

## Errors
A region or table that was not captured leaves its output empty (or as it was) for that tick; a datum of unknown kind is skipped.

## Requirements
Pure: no process, no address (checked by `Tools/Tests/test_components.py`). Deterministic, like the rest of the pipeline.

## Tests
`RuntimeTests` (hand-made Snapshots: a biped, a weapon, a player, the event ring) in `Artemis.Tests`, and `ReplayTests.Recordings`.
