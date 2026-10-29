# Artemis.Views

Group `3_Domain` - pure static library - what one agent can know, built from the omniscient world.

## Responsibility
Builds the view of the agent's local player: its self (the player it controls and where it is), the affordances around it (what it
can pick up, enter or use) and the raycasts it sees with.

## Does not
- Decide actions: that is the Python agent.
- Build the action: that is `Control`.
- Flatten anything for Python: that is `Bridge.Codec`.

## Input and output
- Input: the stores of `Perception::Layer`, `Blam::Runtime::Layer` and `Blam::MapModel::Layer` (world and raycaster).
- Output: `SelfStore`, `AffordanceStore` and `RaycastStore`, members of `Views::Layer`, one published value per tick.

## Public interface
Other projects may import only these modules; everything else in the project is internal.

| Module | What it gives |
|---|---|
| `Views.Layer` | The facade: stores and services; `Cleanup()` forgets the map. |
| `Views.Self.Type`, `Views.Self.State` | The agent's player (`k_SelfLocalIndex`) and its position. |
| `Views.Affordance.Type`, `Views.Affordance.State` | What the agent can interact with, and how. |
| `Views.Raycast.Type`, `Views.Raycast.State` | The rays of the agent and what they hit. |

## Depends on
`Common`, `Blam.Layout`, `Blam.Map`, `Blam.MapModel`, `Blam.Runtime`, `Perception`.

## State between ticks
None: every store holds only the value of the last tick.

## Threads
Updates run on the AI thread, called by `Bridge.Codec.Pipeline.System` after `Perception`. Nothing blocks.

## Errors
An agent without a unit (dead, not spawned) gets an empty view for that tick.

## Requirements
- One agent per process today (the local player `k_SelfLocalIndex`); the same rules must hold for every agent once there are more.
- Deterministic and pure, like the rest of the pipeline.

## Tests
`PipelineTests` and `ReplayTests.Recordings` in `Artemis.Tests` (the views reach the flattened tick).
