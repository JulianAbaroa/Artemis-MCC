# Artemis.Blam.MapModel

Group `2_Blam` - pure static library - what Artemis needs from a map, built once per map and frozen.

## Responsibility
From the raw tag catalog of `Blam.Map`, builds the static model of the loaded map: the definitions (the gate to tag fields: a field
enters when someone reads it), the world (object to model to collision, region states, model links, the hierarchy rays are cast
against), the vitality layout of every model with damage sections, and the resolver of the game's tag handles.

## Does not
- Read game memory or know live objects: that is `Blam.Runtime`.
- Read the file: that is `Blam.Map`. Nothing outside this project reads raw tag structs.

## Input and output
- Input: a loaded `Blam::Map::Layer`.
- Output: the stores of `Blam::MapModel::Layer` (`DefinitionsStore`, `WorldStore`, `VitalityStore`), frozen once built, and
  `TagResolverService`.

## Public interface
Other projects may import only these modules; everything else in the project is internal.

| Module | What it gives |
|---|---|
| `Blam.MapModel.Layer` | The facade: stores, builders and the tag resolver; `Cleanup()` forgets the map. |
| `Blam.MapModel.Definitions.Type`, `Blam.MapModel.Definitions.State` | Resolved definitions by tag group (object, model, damage, level...). The instant responses of a model name their damage effect and the damage effect carries its radius (D259). |
| `Blam.MapModel.Definitions.Reflect` | Reflection of the definitions, for the inspector. |
| `Blam.MapModel.World.Type`, `Blam.MapModel.World.State`, `Blam.MapModel.World.System` | Links of the world and the raycaster. |
| `Blam.MapModel.Vitality.Type`, `Blam.MapModel.Vitality.State` | Vitality layouts by model. |
| `Blam.MapModel.Tag.System` | `TagResolverService`: handle to name and group, `HasColl`/`HasHlmt`/`HasMode`. |

## Depends on
`Common`, `Blam.Map`.

## State between ticks
None: rebuilt for every map, read-only in between.

## Threads
Built on the AI thread when the map loads; afterwards read from any thread without locks (the stores assert they are frozen).

## Errors
A missing or inconsistent tag leaves its field empty with a WARNING; a map is never aborted as a whole. The resolver returns an
invalid `TagInfo` when no map is loaded or the salt does not match.

## Requirements
The only door to raw tags for the rest of the system.

## Tests
`MapModelTests` (forge_halo and shared: load lines, known definitions, every handle of the index, `ScnrBuilder` copies the `MapId` of the scnr), `TagResolverTests` (hand-made
index, no `.map`) and `ObjectKindTests`, in `Artemis.Tests`.
