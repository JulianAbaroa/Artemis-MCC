# Artemis.Blam.Layout

Group `2_Blam` - pure static library (mostly generated) - the shape of the engine's memory, as data.

## Responsibility
Describes the runtime memory of Halo Reach without reading it: the datum layout of every object type (generated from
`Data/Datum/*.xml`), the offsets of the player, interaction and game structures, the per-tick player action, and the `Snapshot`
type: every live object (`RawObject`) and every memory region (`RegionId`) copied at the start of a tick. It also owns the
`.sdump` format that records a stream of Snapshots.

## Does not
- Read memory or know any address: `Engine.Capture` fills the Snapshot.
- Interpret what was copied: that is `Blam.Runtime`.

## Input and output
Types, offsets and the `.sdump` writer and reader. A Snapshot is immutable once published; every read from it is bounds-checked
and returns zeros (or nothing) when the region or the value is missing.

## Public interface
Other projects may import only these modules; everything else in the project is internal.

| Module | What it gives |
|---|---|
| `Blam.Layout.Object.Type` | Datum layouts per object type (`Obje`, `Unit`, `Bipd`, `Weap`, `Vehi`...), generated. |
| `Blam.Layout.Player.Type` | Offsets and sizes of the player table and its mapping. |
| `Blam.Layout.Interaction.Type` | Offsets of the interaction table. |
| `Blam.Layout.Game.Type` | Offsets of the game globals (phase, round, round clock, team mapping and scores, game variant), the format of an engine timer (`TimerTicks`, `TimerTicksCeil`, `IsTimerRunning`), the megalo object elements and the event ring. |
| `Blam.Layout.Action.Type` | Layout of the 0xA8-byte player action of a tick and its button bits. |
| `Blam.Layout.Snapshot.Type` | `Snapshot`, `RawObject`, `RegionId`, `Region` and the `.sdump` records. |
| `Blam.Layout.Snapshot.System` | Write and read `.sdump` files (delta against the previous record, byte by byte). |

## Depends on
`Common`.

## State between ticks
None. The `.sdump` writer keeps the previous Snapshot in a `SnapshotWriterState` the caller owns.

## Threads
Types only; a published Snapshot can be read from any thread.

## Errors
Reads outside a region, a datum or a block return zero-initialized values or `nullopt`, never undefined memory. A `.sdump` with
other region sizes is refused; a file without footer is reported as cut.

## Requirements
- Generated files are never edited by hand: change the XML and run `GenerateDatum.py` (`--check` compares byte for byte).
- No pointer or address is used to read: `Region::OriginalAddress` is only for tools and logs.
- Each region is the whole block the engine allocates for it (or the whole array), so a field read later is already in older
  recordings. A new region or a new size raises the `.sdump` version.

## Tests
`LayoutTests` (snapshot reads, bounds) and `SnapshotRecordTests` (`.sdump` round trip) in `Artemis.Tests`; the offsets are
`static_assert`ed against the generated structs.
