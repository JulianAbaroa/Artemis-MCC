# Templates

Runtime ("dynamic") layouts of the objects living in Halo Reach MCC's object table, as opposed to the static tag layouts stored in the `.map` files. Same XML dialect as Assembly plugins, so they can be read, edited and shared the same way.

## Layout

- `Object/obje.xml`: the base object component, datum `0x000..0x1A8`.
- `Object/unit.xml`: the unit component (bipd, vehi, gint), datum `0x1B8..0x9D8`.
- `seed_templates.py`: brings automatic discoveries into the XML (see below).
- `Tags/ObjectTags.xml`: catalog of object tags with their semantic Function and Subtype (see "Tag catalog").
- `seed_tags.py`: builds and merges the tag catalog.
- `Semantic/Semantic.xml`: single source of the semantic ids (functions, subtypes, groups, states, capabilities, events). See "Semantic ids".

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

       python Templates/seed_templates.py [obje unit bipd ...] [--dry-run]

   - No XML yet for the component: creates it with every candidate as `Unknown XXX`.
   - XML exists: **merges**. Adds candidates that do not overlap an existing field and refreshes the type guess and tooltip of fields still `confidence="unknown"`. Anything else is never touched or deleted. Run it as often as the analysis improves.
   - Component ranges (start, end) are in the `RANGES` table of the script; the XML wins if they differ.
3. Label what you measured or decompiled (edit the XML).
4. Generate the C++:

       python TagTranspiler/GenerateTemplate.py [Templates/Object/unit.xml]

   Writes under `Artemis/Core/Domain/3_Template/Object/Type/`:
   `<Prefix>/Template<Prefix>Offset.ixx` (absolute offsets), `<Prefix>/Template<Prefix>Structure.ixx` (packed struct relative to `start`, `static_assert` on the size) and the aggregate `Template.Object.Type.<Prefix>.ixx`, module `Template.Object.Type`. The `Template` prefix in the file names avoids clashes with the `Map*` files of layer 1 (same `.ixx` name twice in a project).

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

       python Templates/seed_tags.py "%LOCALAPPDATA%\Artemis\TagCatalog" [--dry-run]

   - `group`, `kind` and `mp` come from the engine and are always refreshed. A change between maps is reported.
   - `function`, `subtype` and `confidence` are only written while `confidence` is `unknown` or `seed`. `seed` comes from the old hardcoded lists of `Common.Tag.Type`.
   - Nothing is deleted. `maps` accumulates every map where the tag was seen.
   - A tag is identified by `group` + `name`: the same name can exist in several groups (campaign maps have scenery copies of weapons).
   - `python Templates/seed_tags.py --seed-only` applies only the seed.
3. Label by hand: set `function`/`subtype` and change `confidence` to `high`, `medium` or `low`. That protects the tag from the script.

## Semantic ids

`Semantic/Semantic.xml` declares every semantic enum of Artemis with an explicit id: `Group`, `Function` (with its `group` and its `subtype` list), `Lifecycle`, `Binding`, `Physics`, `Relation`, `Event` and the `Capability` bits. **Ids are stable: only append, never renumber or reuse** (the export to Python and the recordings depend on them).

    python TagTranspiler/GenerateSemantic.py

Writes the module `Common.Semantic.Type` (primary in `Artemis/Core/Common/Semantic/`, partitions under `Type/`): the enums (`:Enum`), one subtype enum per function (`:Subtype`) and the catalog labels with confidence `high`, `medium` or `low` (`:Catalog`, sorted, with `FindTagLabel`). It fails without writing anything when the catalog uses a function or subtype that `Semantic.xml` does not declare. Run it after editing `Semantic.xml` or labeling the catalog.
