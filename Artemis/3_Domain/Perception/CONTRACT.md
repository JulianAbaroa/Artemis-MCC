# Artemis.Perception

Group `3_Domain` - pure static library - builds the world as the engine knows it, once per tick.

## Responsibility
Turns the live tables of one tick into the omniscient picture of the world (the engine's truth): it classifies the live objects
and relates them in the object graph, then builds the details (vitality, zones, teleporters, lifts, barriers, explosives, projectiles, weapons, objectives), the entities, the
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
- Output: one store per result, members of `Perception::Layer` (`ClassifierStore`, `ObjectGraphStore`, the seven details stores,
  `EntitiesStore`, `PlayersStore`, `AimStore`, `EventsStore`, `MatchStore`). Each store publishes an immutable value per tick.

## Public interface
Other projects may import only these modules; everything else in the project is internal.

| Module | What it gives |
|---|---|
| `Perception.Layer` | The facade: owns the stores and services; `Cleanup()` forgets the map. |
| `Perception.Classifier.Type`, `Perception.Classifier.State` | Kind, function and group of each live object. |
| `Perception.ObjectGraph.Type`, `Perception.ObjectGraph.State` | How the live objects attach to each other, up to their root. |
| `Perception.Details.Type`, `Perception.Details.State` | Vitality, zone, teleporter, lift (the stages of the phantom types of its tag and the measured exit of the last biped it pushed, D98, D261), barrier, explosive (radius and damage of the explosion of its tags, D259), projectile (owner, shooter root, what it is stuck to), weapon (ammo, energy, heat, overheat, reload, charge, lock, `CanFireNow`) and objective details (flags and their stands). |
| `Perception.Entities.Type`, `Perception.Entities.State` | The entities of the tick. The team of a biped is the one of its player and the team of a projectile the one of its `OwnerPlayer` in the player table. |
| `Perception.Players.Type`, `Perception.Players.State` | The players of the tick. `RespawnTicks`, `Traits` (the traits the engine applies in this tick, D262, the `PlayerTraits` type of Blam.Layout), `Powerup` (Common Semantic Subtype Powerup) and `PowerupTicks` (D263) belong to the player and not to its `Body` (valid without a biped). `PlayersService` computes the powerup with `ActivePowerup` of Blam.Layout from the pickups of the player table and the game time (`EngineTick`) and powerup durations of the game store read in the same `Update` (None without game). The life of a biped is `Entity.Vitality`, there is no `Body.Health`. `Body` carries the ground and the crouch of the biped (D260): `Ground` (`GroundKind`: Air, Static, Object, Attached), `IsGrounded`, `AirborneTicks`, `GroundObject`, `OwnVelocity` (relative to the object under the feet), `CrouchFraction` and `IsCrouching`. A contact that cuts for 1 or 2 ticks keeps the last ground and a jump releases it at once. |
| `Perception.Aim.Type`, `Perception.Aim.State` | What each player aims at. |
| `Perception.Events.Type`, `Perception.Events.State` | The events of the tick: the engine ring plus the flag events Artemis generates. |
| `Perception.Match.Type`, `Perception.Match.State` | Status of the match: phase (InGame until the GameOver event of the engine, then Ended), scores, round, team scores, time played and left in the round, round time limit, round limit, score to win and the respawn time, lives, rounds to win, sudden death, grace period and powerup durations of the game variant (in ticks, D262). |
| `Perception.Collidable.Type`, `Perception.Collidable.System` | Collision geometry of objects, on demand (used by `Views` and the viewer). |

## Depends on
`Common`, `Blam.Layout`, `Blam.Map`, `Blam.MapModel`, `Blam.Runtime`.

## State between ticks
- `CarriedHandlesStore`: every object a biped carried since the map loaded (a dropped Fixed or Phased item can never be picked up
  again, so `IsPickable` needs this history).
- `TagClassificationsStore`: the classification of each tag of the loaded map, computed once. The per-object result can differ from
  it in one case: a zone of any tag attached to a flag is classified `ObjectiveZone/FlagZone` (the zone where allies return it; the tag of a gametype may call it a hill).
- `EventsStore`: a log of the last events, so readers that sample less often than every tick (the bridge, the bench) miss
  none. `GameEvent.Sequence` is the sequence of Artemis (from 1 per match, no gaps, engine events of the tick first, then flag events)
  and `EngineSequence` the one of the engine ring (0 in the flag events Artemis generates). The engine's own flag events are left out.
  The cursor of the engine's event ring lives in `Blam.Runtime`.
- `WeaponChargeMemoryStore`: the first engine tick each weapon was seen in the Charging state, by handle, for `ChargeFraction` (the engine
  gives the stage but not how long it has lasted). Only the weapons that are charging are in it. It is emptied on `Cleanup()` (a load) and a
  weapon seen already charging starts from that tick. With a tick delta greater than 1 (sparse updates) the first tick seen charging is the
  sample, so `ChargeFraction` under-estimates by up to delta - 1 ticks. It always counts from `EngineTick`, never from a number of updates.
- `GroundMemoryStore`: by biped handle, the ticks without contact, the ground and object of the last contact and whether the biped jumped off it, to
  resolve `Body.Ground` (D260). Only the bipeds with a `Body` in the last tick are in it. It is emptied on `Cleanup()` and a new biped starts without
  history (its `AirborneTicks` count from 0). Each entry keeps the `EngineTick` of the update that wrote it and ages by the ticks since then
  (1 for a repeated tick or without game). With a delta greater than 1 the contact may have ended at any tick of the interval, so the latest is taken:
  the first sample without contact is `AirborneTicks` 1 (and `IsTakeoff` is evaluated there), the next ones add the delta. A clock that goes
  back leaves the biped without history. With delta 1 the result is the one of every tick.
- `LiftMemoryStore`: the bipeds that were inside a phantom volume in the last tick with the lift each one was assigned when it entered (the nearest
  one within 3 world units, none for doors and other phantoms) and the last exit of each lift (tick, velocity and biped), for `LiftDetail.HasExit`
  (D261). The entry is the first sample inside the volume and the exit the first sample outside, with the tick and velocity of that sample
  (a pass through the volume shorter than the sampling interval is not seen). A biped that disappears inside is forgotten without an exit and a lift that disappears loses its exit. It is emptied on `Cleanup()`.
- `EventsMemoryStore`: the last flag seen of each base team, the round, the last engine sequence and the engine tick in which the GameOver event was
  read, to generate the flag events on the transitions (none when the round changes or on the first tick after a load). The Match service reads
  the GameOver tick to derive `MatchStatus.Phase`, never the raw phase of the engine (it is also Ended between rounds). A new engine (the clock or
  the ring sequence go back) forgets it.
- `CarriedHandlesStore` only adds what is seen carried in the sample: it has no tick count and needs no delta.
- Every other store holds only the value of the last tick. `Cleanup()` forgets the map.

## Threads
Updates run on the AI thread, after the read window closed: the game is not waiting for them. Nothing blocks and no thread is
created. `Cleanup()` runs on the lifecycle thread: the stores that keep history only mark themselves and empty on the next update.
Other threads (development tools) read the published stores without locks.

## Errors
A missing or invalid table entry leaves the results that depend on it empty for that tick; nothing throws.

## Requirements
- Deterministic: the same Snapshot and map give the same output bit for bit (the replay test depends on it).
- Fixed order inside the tick: Classifier, ObjectGraph, Vitality, Zone, Teleporter, Lift, Barrier, Explosive, Projectile, Weapon, Entities, Players, Objective, Aim, Events,
  Match.
- Pure: no process, no `windows.h`, no wiring of the lifecycle (checked by `Tools/Tests/test_components.py`).

## Tests
- `ReplayTests.Recordings`: a recorded Snapshot stream run through the whole pipeline must give the golden `.tdump` byte for byte.
- `PipelineTests` (the pure chain without the DLL), `CollidableSceneTests`, `ClassifierFlagZoneTests` and `LiftDetailsTests` (stages, who each one affects, the measured exit), in `Artemis.Tests`.
- `Tools/Tests/test_components.py`: imports, references and purity of the project.
