# Artemis.Blam.Layout

Group `2_Blam` - pure static library (mostly generated) - the shape of the engine's memory, as data.

## Responsibility
Describes the runtime memory of Halo Reach without reading it: the datum layout of every object type (generated from
`Data/Datum/*.xml`), the offsets of the player, interaction and game structures, the per-tick player action, and the `Snapshot`
type: every live object (`RawObject`) and every memory region (`RegionId`) copied at the start of a tick. It also owns the
`.sdump` format that records a stream of Snapshots, and the flags and result of the engine ray test (`Collision`).

## Does not
- Read memory or know any address: `Engine.Capture` fills the Snapshot.
- Interpret what was copied: that is `Blam.Runtime`.

## Input and output
Types, offsets and the `.sdump` writer and reader. A Snapshot is immutable once published (its `RawObject` views point into the arena of its table: a table that is copied, cleared or added to rebases or drops them, so they are not kept across changes); every read from it is bounds-checked
and returns zeros (or nothing) when the region or the value is missing.

## Public interface
Other projects may import only these modules; everything else in the project is internal.

| Module | What it gives |
|---|---|
| `Blam.Layout.Object.Type` | Datum layouts per object type (`Obje`, `Unit`, `Bipd`, `Weap`, `Vehi`...), generated. |
| `Blam.Layout.Player.Type` | Offsets and sizes of the player table and its mapping, and the block of player traits (`PlayerTraits`, its enums and the raw-index tables, `Decode` of the 0x2C bytes at player +0x328, D262), and the custom powerup (`ActivePowerup`: the rule of the engine from the three pickup game times at player +0x300, the game time and the durations of the variant, D263). |
| `Blam.Layout.Interaction.Type` | Offsets of the interaction table. |
| `Blam.Layout.Collision.Type` | What the raycast oracle needs of the engine function `collision_test_vector` (R3, D272): the flag bits of its `u64` (`k_Static`, `k_StaticAndObjects`, with every surface and both faces), the size and offsets of its 0x70-byte result, and `CollisionResult` with `Decode` of those bytes. |
| `Blam.Layout.Game.Type` | Offsets of the game globals (phase, round, round clock, team mapping and scores, game variant with its type, score to win, respawn, round and powerup settings, Megalo trait sets), the format of an engine timer (`TimerTicks`, `TimerTicksCeil`, `IsTimerRunning`), the megalo object elements and the event ring. Also the pure logic of the gametype presets (`:VariantPreset`, namespace `VariantPreset`, Q97, D264, D265): `ParseCatalog` and `ParsePreset` read the compiled files of `Tools/presets.py` (a wrong line invalidates the whole preset), `SaveBase` keeps the value of every field of the catalog, `Compose` gives the final value of each field (preset or saved base), `Plan` decides a request (rejected, with a reason and nothing to write, when the variant is not Megalo, a trait set does not exist, or between rounds a `Load` field would change) and returns the writes that change the variant, `Apply` writes them with their masks and `Verify` reads them back. It works on bytes it is given: the engine writes nothing here. |
| `Blam.Layout.Action.Type` | Layout of the 0xA8-byte player action of a tick and its button bits. |
| `Blam.Layout.Snapshot.Type` | `Snapshot`, `RawObjectTable` (the live objects of a Snapshot, D276: the datums one after the other in one arena, an index by engine slot, iteration in the order they were added, `Find` checks the whole handle salt included), `RawObject` (a view of one datum inside that arena: `Read`, `ReadRaw`, `FindBlock` and `ReadBlock` are bounds-checked and give zero when the value does not fit), `RegionId`, `Region` and the `.sdump` records. The format of the `.sdump` did not change. A light Snapshot (`IsLight`, Q101) holds only the objects of kind `k_LightKindId` (the weapons, which include the flags) and the regions `IsLightRegion` says (`TimeGlobals`, `GameEngineGlobals`, `MegaloObjects`): it is never written to a `.sdump` and the full pipeline never runs on it. |
| `Blam.Layout.Snapshot.System` | Write and read `.sdump` files (delta against the previous record, byte by byte), and `MakeLight`: the light Snapshot of a full one with the same content `CaptureService::CaptureLight` copies from the game (the offline mode of the sparse tests). |

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
- The fixed size of the nine concrete types the engine allocates (Bipd 0xE00, Vehi 0xDE8, Weap 0x338, Eqip 0x210, Proj 0x258, Scen 0x1E4, Bloc 0x1B0, Mach 0x2D4, Ctrl 0x2CC) is a table in `GenerateDatum.py` (`EXPECTED_END`), and the generated Offset partition `static_assert`s `k_ComponentEnd` against it.
- No pointer or address is used to read: `Region::OriginalAddress` is only for tools and logs.
- Each region is the whole block the engine allocates for it (or the whole array), so a field read later is already in older
  recordings. A new region or a new size raises the `.sdump` version.

## Tests
`LayoutTests` (snapshot reads, bounds) and `SnapshotRecordTests` (`.sdump` round trip) in `Artemis.Tests`; the offsets are
`static_assert`ed against the generated structs.
