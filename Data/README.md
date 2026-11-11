# Data

Sources of truth of Artemis: hand-edited XML, committed, with nothing executable next to them. The scripts that read and write them live in `Tools/` (generators in `Tools/TagTranspiler/`, seeds in `Tools/`).

`Datum/` holds the runtime ("dynamic") layouts of the objects living in Halo Reach MCC's object table, as opposed to the static tag layouts stored in the `.map` files. Same XML dialect as Assembly plugins, so they can be read, edited and shared the same way.

## Layout

- `Datum/obje.xml`: the base object component, datum `0x000..0x1A8`.
- `Datum/unit.xml`: the unit component (bipd, vehi, gint), datum `0x1B8..0x9D8`.
- `Datum/mvar.xml`: one placement of the map variant (Forge settings of a placed object, Physics included), `0x4C` bytes. Not part of the datum: the object points to it with obje `0x1C`.
- `../Tools/seed_datum.py`: brings automatic discoveries into the XML (see below).
- `Tags/ObjectTags.xml`: catalog of object tags with their semantic Function and Subtype (see "Tag catalog").
- `../Tools/seed_tags.py`: builds and merges the tag catalog.
- `Semantic/Semantic.xml`: single source of the semantic ids (functions, subtypes, groups, states, capabilities, events). See "Semantic ids".
- `Presets/Catalog.xml` and `Presets/<name>.xml`: the writable fields of the game variant and the gametype presets. See "Presets".

## Format

Root: `<plugin game runtime="true" component start baseSize>`. `start` is the first datum byte of the component, `baseSize` its end. Field offsets are **absolute datum offsets**.

Per field: `name`, `offset`, `tooltip`, plus these extra attributes (ignored by Assembly):

| attribute | meaning |
|---|---|
| `confidence` | `high` (measured, or decompiled and verified), `medium`, `low`, `unknown` (only access statistics) |
| `default` | value the engine writes at creation (when known) |
| `exposed` | `true` if Artemis reads it every tick |

Fields named `Unknown XXX` come from the automatic analysis; their tooltip lists reads (R), writes (W), initial values and writer functions. Rename them as you identify them, fix the type if needed and **change `confidence` away from `unknown`**: that is what protects the field from the script.

**The XML is the source of truth.** Edit it by hand and commit it.

## Workflow

1. Extract (needs Ghidra and the game's DLL, lives outside the repo in `Ignore/Ghidra/`):
   `ExportObjectTemplates.java` (Ghidra script) -> `analyze_templates.py` -> `Ignore/Ghidra/templates/fields_<comp>.txt`.
2. Bring the discoveries into the XML:

       python Tools/seed_datum.py [obje unit bipd ...] [--dry-run]

   - No XML yet for the component: creates it with every candidate as `Unknown XXX`.
   - XML exists: **merges**. Adds candidates that do not overlap an existing field and refreshes the type guess and tooltip of fields still `confidence="unknown"`. Anything else is never touched or deleted. Run it as often as the analysis improves.
   - Component ranges (start, end) are in the `RANGES` table of the script; the XML wins if they differ.
3. Label what you measured or decompiled (edit the XML).
4. Generate the C++:

       python Tools/TagTranspiler/GenerateDatum.py [Data/Datum/unit.xml]

   Writes under `Artemis/2_Blam/Layout/Datum/Type/`:
   `<Prefix>/Datum<Prefix>Offset.ixx` (absolute offsets), `<Prefix>/Datum<Prefix>Structure.ixx` (packed struct relative to `start`, `static_assert` on the size) and the aggregate `Blam.Layout.Object.Type.<Prefix>.ixx`, module `Blam.Layout.Object.Type`. The `Datum` prefix in the file names avoids clashes with the `Map*` files of layer 1 (same `.ixx` name twice in a project).

## Notes

- XML comments are lost when the script rewrites a file; put notes in `tooltip`.
- `Old name:` in a tooltip points to the name the field had in `ObjectOffset.ixx`.

## Inheritance

Every object type is built by stacking components in the datum; each component always occupies the same range. `obje` (0x0..0x1A8) is in all of them.

| type | components | fixed size |
|---|---|---|
| biped | obje + anim + unit + bipd | 0xE00 |
| vehicle | obje + anim + unit + vehi | 0xDE8 |
| weapon | obje + item + weap | 0x338 |
| equipment | obje + item + eqip | 0x210 |
| terminal | obje + devi + term | 0x2C8 |
| projectile | obje + proj | 0x258 |
| scenery | obje + scen | 0x1E4 |
| machine | obje + devi + mach | 0x2D4 |
| control | obje + devi + ctrl | 0x2CC |
| sound scenery | obje + ssce | 0x1DC |
| crate | obje + bloc | 0x1B0 |
| creature | obje + anim + crea | 0x294 |
| giant | obje + anim + unit + gint | 0x3BE0 |
| effect scenery | obje + efsc | 0x1AC |

A field labeled in a component applies to every type that includes it.

## Tag catalog

`Tags/ObjectTags.xml` gives every object tag its semantic `function` and `subtype` (design in `Ignore/Semantizacion/05`, section 3: catalog > multiplayer object kind of the tag > kind default). **It is the source of truth for those two fields.**

Per tag: `name`, `group` (fourcc), `kind` (ObjectKind), `mp` (multiplayer object kinds of the tag), `function`, `subtype`, `confidence`, `maps`.

1. Load a map in the game and press **Dump object tag catalog of this map** in the Definitions Inspector (UI, section "Dump for field-usage report"). Artemis writes `%LOCALAPPDATA%\Artemis\TagCatalog\<map>.xml` with group, kind and multiplayer object kinds of **every object tag of the map file** (bipd, bloc, ctrl, eqip, mach, proj, scen, vehi, weap; not only the objects alive in the object table). Needs "Enable Local Storage (AppData)" in Settings; the folder is not configurable. The file is named after the scenario tag of the map. Every Reach map was already dumped and merged (2026-10-12); repeat only if a map changes.
2. Merge the dumps:

       python Tools/seed_tags.py "%LOCALAPPDATA%\Artemis\TagCatalog" [--dry-run]

   - `group`, `kind` and `mp` come from the engine and are always refreshed. A change between maps is reported.
   - `function`, `subtype` and `confidence` are only written while `confidence` is `unknown` or `seed`. `seed` comes from the old hardcoded lists of `Common.Tag.Type`.
   - Nothing is deleted. `maps` accumulates every map where the tag was seen.
   - A tag is identified by `group` + `name`: the same name can exist in several groups (campaign maps have scenery copies of weapons).
   - `python Tools/seed_tags.py --seed-only` applies only the seed.
3. Label by hand: set `function`/`subtype` and change `confidence` to `high`, `medium` or `low`. That protects the tag from the script.

## Semantic ids

`Semantic/Semantic.xml` declares every semantic enum of Artemis with an explicit id: `Group`, `Function` (with its `group` and its `subtype` list), `Lifecycle`, `Binding`, `Physics`, `Relation`, `Event` and the `Capability` bits. **Ids are stable: only append, never renumber or reuse** (the export to Python and the recordings depend on them).

    python Tools/TagTranspiler/GenerateSemantic.py

Writes the module `Common.Semantic.Type` (primary in `Artemis/1_Shared/Common/Semantic/`, partitions under `Type/`): the enums (`:Enum`), one subtype enum per function (`:Subtype`) and the catalog labels with confidence `high`, `medium` or `low` (`:Catalog`, sorted, with `FindTagLabel`). It fails without writing anything when the catalog uses a function or subtype that `Semantic.xml` does not declare. Run it after editing `Semantic.xml` or labeling the catalog.

## Presets

`Presets/` holds the gametype presets (Q97, D264, D265): sets of values that Artemis writes into the game variant `V` (the one the engine reads: respawn time, traits, rounds, loadouts...) at load and between rounds. The XML is the source of truth, hand-edited and committed. The DLL does not read XML: `Tools/presets.py` compiles it into text files and editing a preset needs no rebuild.

- `Catalog.xml`: every writable field once. `<enum>` (names and values of ReachVariantTool, such as `75%` or `rocket_launcher`), one `<traits>` block (the 0x2C bytes of player traits with their subfields, used by every group with `traits="true"`) and `<group>` elements. A group has an `offset` (from the start of `V`, the offsets of its fields are relative to it), a default `when` and optional `<index name count stride>` for repeated blocks (`team` 8 x 0x58, `loadout` palette 6 x 30 and index 5 x 6, `trait_set` 16 x 0x30). A `<field>` has `name`, `offset`, `type` (`u8 s8 u16 s16 u32 s32 bit`), `enum`, `unit`, `doc`, `when` and, for `bit`, `bit` and `width`. `when` says when the engine reads the field: `tick`, `spawn`, `pickup`, `round_start`, `round_end` or `load` (only at load, a request between rounds that would change it is rejected). Fields not measured yet are `load` (A19 section 5.1).
- `<name>.xml`: one preset per file. The name is the file name (`[a-z0-9_]`, up to 31 characters).

      <preset base="artemis_ctf_base" doc="...">
        <general round_limit="3" time_limit_minutes="2" />
        <base_traits gravity="75%" movement_speed="110%" jump_height="100" />
        <trait_set index="0" movement_speed="90%" />
        <team index="1" primary_color="0x00FF00" />
      </preset>

  One element per group or block, one attribute per field. Only the fields written are set, the rest keeps the value of the gametype (D265 b). `base` is another preset: the child overrides the parent field by field, and chains are allowed (no cycles). A value is the name of the enum or a number (`0x` for hex) inside the range of the type; a `bit` takes `0`, `1`, `true` or `false`. An error is reported as `file:line` with what was expected and invalidates the whole preset.
- `artemis_ctf_base.xml` is the Artemis CTF as measured in Forge `artemis_ctf_fast.xml` a child for short training rounds and `artemis_ctf_colors.xml` a child that sets the colors of teams 0 and 1 (a field that is only read at load: a request between rounds is rejected). The bench makes a preset active with the match key `preset <name>` (or `preset none`) and checks it with `preset_applied [<name>]`.

      python Tools/presets.py compile [--out DIR]   writes <name>.txt and _catalog.txt (default build/game/presets)
      python Tools/presets.py --check [--out DIR]   exit 1 when the compiled files differ from the XML
      python Tools/presets.py list

`game.py deploy` and `game.py run` compile first, and the game watcher copies the files to `%LOCALAPPDATA%\Artemis\Presets`. A compiled preset is one line per write (`write <offset hex> <when> <bytes hex> <mask hex>`, inheritance resolved, only the bits of the mask are written) plus `preset`, `source` (hash of the XML and its bases) and `check megalo_trait_set <i>` lines. `_catalog.txt` lists every field (`field <offset hex> <size> <mask hex> <when>`): the fields Artemis saves at load and restores when the preset changes. The parse and the application of these files is `Blam.Layout.Game.Type:VariantPreset`; `Artemis/7_Tests/Tests/Blam/Fixtures/presets` is the fixture both sides test against (regenerate it as `test_presets.py` says).

## Map variants

A map variant (Q98, D266) is the Forge layout of the same `.map`: the objects Forge placed, their spawns and the labels the gametype searches for. Artemis can put one in at load, so a match trains on another layout without the lobby. It lives in the recorded match configurations, not in `Data/`: there is nothing to compile.
- The name of a map variant is the name of a recorded configuration (`%LOCALAPPDATA%\Artemis\MatchConfigs\<name>.bin`, D123) and the map variant is the 0xD9BC-byte block of that configuration at +0x1D308. To add a map, record a configuration from the lobby once, with that map variant selected.
- The bench makes one active with the match key `map <config name>` (or `map none`, which removes it; no key leaves the active one) and checks it with `map_applied [<config name>]`. In a scenario JSON the key is `"map"` (`null` = none). It is persistent like the preset and independent of it: any preset goes with any map variant.
- At load the block goes into the copy of the start configuration after the transplant and before the preset. It is written only if it fits the one the copy carries: same map id, version 0x20 and same number of quotas. If it does not, the log has a WARNING and the start keeps the map variant of its configuration (`map_applied` fails and the Workbench line says `rejected`).

## Bot rosters

A roster (LS.1, D284) is how many bots each team has plus how many go to the team of the local player 0 (`auto`). The local players are not part of it.
- The bench takes it with the match key `roster <t0> ... <t7> auto <n>` and one `round_roster ...` line per next round. In a scenario JSON: `"roster": {"0": 7, "1": 8, "auto": 0}` and `"rosters": [r1, r2, ...]`. `"bots": [0, 1, "auto"]` is a shortcut that game.py turns into a roster (a count per team); a match with `roster` and `bots` is an error, and the one of the match replaces the one of the defaults. A match without any has no bots: the roster of the previous match is cleared. `rosters` need a roster at load.
- The roster of the match is applied at the first tick. `r_k` is asked for while round k - 1 runs and applied at the end of that round, so it is the roster of round k. The plan keeps the bots a team has up to what it wants, moves the extra ones to the teams that miss some, removes the rest and adds what is still missing. A roster that does not fit the 16 player slots is refused whole.
- The check `roster_ok` compares, at the start of every round in play, the bots per team of the Tick with the roster of that round. The report lists round, expected and seen.
