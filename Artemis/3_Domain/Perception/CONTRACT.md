# Artemis.Perception

Group `3_Domain` - pure static library - builds the world as the engine knows it, once per tick.

## Responsibility
Turns the live tables of one tick into the omniscient picture of the world (the engine's truth): it classifies the live objects
and relates them in the object graph, then builds the details (vitality, zones, teleporters, lifts, barriers, objectives), the entities, the
players, the aim, the events and the match status.

## Does not
- Read the game process, a clock or a file: its only input is `Blam.Runtime` and `Blam.MapModel`.
- Filter by agent: that is `Views`.
- Assemble or flatten the Tick for Python: that is `Bridge.Codec`.
- Decide the order of the tick or time its stages: `Bridge.Codec.Pipeline.System` calls each service in order and `Mod` times them.
- Build the collidable scene of the free camera every tick: `DevTools.Viewer` asks `Perception.Collidable.System` for it on demand.

## Input and output
- Input: the stores of `Blam::Runtime::Layer` (objects, players, damage sections, bone matrices, game) and of
  `Blam::MapModel::Layer` (definitions, vitality layouts, world).
- Output: one store per result, members of `Perception::Layer` (`ClassifierStore`, `ObjectGraphStore`, the five details stores,
  `EntitiesStore`, `PlayersStore`, `AimStore`, `EventsStore`, `MatchStore`). Each store publishes an immutable value per tick.

## Public interface
Other projects may import only these modules; everything else in the project is internal.

| Module | What it gives |
|---|---|
| `Perception.Layer` | The facade: owns the stores and services; `Cleanup()` forgets the map. |
| `Perception.Classifier.Type`, `Perception.Classifier.State` | Kind, function and group of each live object. |
| `Perception.ObjectGraph.Type`, `Perception.ObjectGraph.State` | How the live objects attach to each other, up to their root. |
| `Perception.Details.Type`, `Perception.Details.State` | Vitality, zone, teleporter, lift, barrier and objective details (flags and their stands). |
| `Perception.Entities.Type`, `Perception.Entities.State` | The entities of the tick. |
| `Perception.Players.Type`, `Perception.Players.State` | The players of the tick. |
| `Perception.Aim.Type`, `Perception.Aim.State` | What each player aims at. |
| `Perception.Events.Type`, `Perception.Events.State` | The events of the tick: the engine ring plus the flag events Artemis generates. |
| `Perception.Match.Type`, `Perception.Match.State` | Status of the match: phase, scores, round and team scores. |
| `Perception.Collidable.Type`, `Perception.Collidable.System` | Collision geometry of objects, on demand (used by `Views` and the viewer). |

## Depends on
`Common`, `Blam.Layout`, `Blam.Map`, `Blam.MapModel`, `Blam.Runtime`.

## State between ticks
- `CarriedHandlesStore`: every object a biped carried since the map loaded (a dropped Fixed or Phased item can never be picked up
  again, so `IsPickable` needs this history).
- `TagClassificationsStore`: the classification of each tag of the loaded map, computed once.
- `EventsStore`: a log of the last events, so readers that sample less often than every tick (the bridge, the bench) miss
  none. `GameEvent.Sequence` is the sequence of Artemis (from 1 per match, no gaps, engine events of the tick first, then flag events)
  and `EngineSequence` the one of the engine ring (0 in the flag events Artemis generates). The engine's own flag events are left out.
  The cursor of the engine's event ring lives in `Blam.Runtime`.
- `EventsMemoryStore`: the last flag seen of each base team, the round and the last engine sequence, to generate the flag events on
  the transitions (none when the round changes or on the first tick after a load).
- Every other store holds only the value of the last tick. `Cleanup()` forgets the map.

## Threads
Updates run on the AI thread, after the read window closed: the game is not waiting for them. Nothing blocks and no thread is
created. `Cleanup()` runs on the lifecycle thread: the stores that keep history only mark themselves and empty on the next update.
Other threads (development tools) read the published stores without locks.

## Errors
A missing or invalid table entry leaves the results that depend on it empty for that tick; nothing throws.

## Requirements
- Deterministic: the same Snapshot and map give the same output bit for bit (the replay test depends on it).
- Fixed order inside the tick: Classifier, ObjectGraph, Vitality, Zone, Teleporter, Lift, Barrier, Entities, Players, Objective, Aim, Events,
  Match.
- Pure: no process, no `windows.h`, no wiring of the lifecycle (checked by `Tools/Tests/test_components.py`).

## Tests
- `ReplayTests.Recordings`: a recorded Snapshot stream run through the whole pipeline must give the golden `.tdump` byte for byte.
- `PipelineTests` (the pure chain without the DLL) and `CollidableSceneTests`, in `Artemis.Tests`.
- `Tools/Tests/test_components.py`: imports, references and purity of the project.
