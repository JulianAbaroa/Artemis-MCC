# Templates

Runtime ("dynamic") layouts of the objects living in Halo Reach MCC's object table, as opposed to the static tag layouts stored in the `.map` files. Same XML dialect as Assembly plugins, so they can be read, edited and shared the same way.

## Layout

- `Object/obje.xml`: the base object component, datum `0x000..0x1A8`.
- `Object/unit.xml`: the unit component (bipd, vehi, gint), datum `0x1B8..0x9D8`.
- `seed_templates.py`: brings automatic discoveries into the XML (see below).

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
