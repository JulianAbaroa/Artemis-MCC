# `.map` file format - Halo Reach (MCC, Update 13)

Technical guide to the internal structure of Halo Reach `.map` files in the
Master Chief Collection (build U13 / `CacheType.MccHaloReachU13`). It describes
how the data is laid out on disk, the segments the file is divided into, the
addressing schemes used to navigate them, and the tag/group/block model that all
data in the file is built from.

The purpose of this document is to capture the format itself, its concepts, its
sections, and its addressing rules, at the level needed to implement a `.map` reader from scratch.

---

## Credits

The complete structure of the `.map` files was made available thanks to the work
of the **Assembly** developers (https://github.com/xboxchaos/assembly), the
reference tool from which all field offsets, struct layouts, and segment
definitions are taken. The addressing and resolution logic was additionally
cross-checked against **Reclaimer** (https://github.com/Gravemind2401/Reclaimer).

The struct/field names in this document come from definitions auto-generated from
Assembly templates.

---

## 1. The mental model: a memory dump, not a file

The single most important fact about a `.map` file is that **it is a snapshot of
the game engine's memory**, written to disk. It was never designed to be read as
a file, it was designed to be *mapped*, copied wholesale into the engine's RAM
at a known address and used directly.

Two consequences follow from this, and they govern everything else:

1. **Pointers inside the file are virtual addresses, not file offsets.** A
   pointer like `0x80001000` does not mean "byte `0x80001000` of the file." It
   means "the byte that will sit at virtual address `0x80001000` once the engine
   maps this file into RAM." Reading the file from disk means performing the
   *inverse* of what the engine does: taking a pointer that assumes a memory
   layout and translating it back into a position in the file.

2. **The data is a hierarchy, not a flat table.** To reach a given value you must
   walk the whole chain that leads to it, header → index → table → tag → nested
   block. You cannot seek directly to an arbitrary piece of data, its location is
   only knowable by resolving the pointers that lead there.

A reader is therefore fundamentally an *address translator*. Most of its
complexity is not in reading bytes (that part is trivial) but in converting the
several kinds of stored pointer into real file offsets.

---

## 2. What a `.map` contains

Conceptually, the file holds:

- **Tags**: the unit of content. A tag is a self-contained block of data
  describing one thing: a weapon, a biped, a physics model, a chunk of level
  geometry. Everything the engine loads is a tag.

- **Groups**: the *type* of a tag. A group defines the structure and meaning of
  a tag's data, much like a class defines the layout of an object. Groups are
  identified by a 4-character magic (a FourCC): `vehi` (vehicle), `bipd` (biped),
  `phmo` (physics_model), `sbsp` (structure BSP), and so on. Groups form a
  three-level inheritance hierarchy (group → parent → grandparent).

- **TagNames**: a human-readable path identifying each tag within the content
  hierarchy, e.g. `objects\characters\spartans\spartans` or
  `objects\vehicles\human\falcon\falcon`. Names are not stored alongside the tag
  data, they live in a separate segment (see §4.3).

- **Supporting tables**: localization string tables (multiple languages),
  scripting data (compiled scripts, global variables), and other engine tables
  needed to run the map at runtime.

The relationship is: every tag points to one group (its type), and the group
tells the reader how to interpret the tag's bytes.

---

## 3. Top-level layout

A `.map` file is a **Header** followed by several **segments**. The header is a
fixed-size block at offset 0 that holds everything needed to bootstrap navigation
of the rest of the file. Each segment is an independent region with its own
**addressing scheme**, which is why each requires its own pointer-to-offset
conversion.

```
+--------------------------------------------------+ 0x0
| Header (0xA000 bytes)                            |
|   virtual base, index header ptr,                |
|   per-segment masks and section bounds,          |
|   filename table offsets, scenario name, ...     |
+--------------------------------------------------+
| Tag Segment      (every tag's metadata)          |
+--------------------------------------------------+
| Debug Segment    (tag filenames / strings)       |
+--------------------------------------------------+
| Resource Segment (raw geometry / buffer pages)   |
+--------------------------------------------------+
| (one further segment, not yet identified)        |
+--------------------------------------------------+
```

The header describes the segments through two parallel arrays, each with four
entries:

- **Section table**: the virtual address and size of each segment.
- **Offset mask table**: a per-segment delta used to convert that segment's
  stored pointers into file offsets.

The four index positions correspond to: `[0]` debug, `[1]` resource, `[2]` tag,
`[3]` the unidentified segment. The reader needs the tag and debug entries to
bootstrap, the resource entry to read geometry, and `[3]` is currently unused.

---

## 4. The segments

### 4.1 Tag Segment
Holds the **metadata block** of every tag in the map: the actual fields, tag
references, and tag blocks that make up each tag's data. This segment was built
to be mapped directly into memory, so **every pointer inside it is a virtual
address** and requires the full two-step translation (§5): expand the compressed
form, then rebase the virtual address to a file offset.

This is where `matg`, `scnr`, `sbsp`, `hlmt`, and every other tag's data lives.

### 4.2 Debug Segment
Holds the data needed to reconstruct **tag filenames**, via two tables:

1. **Name offset table**: one int32 per tag, giving the byte offset of that
   tag's name within the name-data blob. A value of `-1` means the tag has no
   name. Indices here line up 1:1 with the tag table.
2. **Name data blob**: a flat buffer of concatenated null-terminated ASCII
   strings. The name of tag `i` is read by seeking to
   `nameDataBase + nameOffset[i]` and reading until the null terminator.

This segment exists purely for tooling, it has no runtime role. Accordingly it
uses a **simpler addressing scheme** than the tag segment: its pointers carry a
fixed bias (a mask from the header) that is simply added, with no virtual-base
subtraction (§5.3).

### 4.3 Resource Segment
Holds the **raw bytes of geometry and other buffer data**, the vertex buffers,
index buffers, and resource pages that higher-level systems (e.g. the mesh
reader) consume. Tags in the tag segment reference this data indirectly: a tag
points into a resource table that resolves, through page/segment indirection, to
a physical position in this segment.

Its pointers are translated with the resource section's own base and mask
(§5.4). Note that the data here may be **compressed** (raw deflate) and may
physically live in an **external cache** (`shared.map`, `campaign.map`) rather
than in the map's own file, resolving a resource therefore can involve
decompression and reading from a different file. The instance/transform data,
however, is always local to the map's own tag segment, only the geometry bytes
are shared.

### 4.4 Unidentified segment
The header reserves a fourth section/mask pair (`[3]`) that is not yet
identified. Based on the surrounding tables it most likely relates to
localization data. Its exact contents remain unconfirmed and no reader path uses
it yet.

---

## 5. Addressing: the four translations

Navigating the file means converting stored pointers into file offsets. There are
distinct schemes because different segments store their pointers differently.

### 5.1 Why translation is needed
When the engine loads the map, it copies the tag segment into RAM at a fixed
**virtual base address** (e.g. `0x80000000`). Every pointer was written assuming
that address. Reading from disk, those pointers are meaningless until rebased to
where the same bytes actually sit in the file.

### 5.2 Tag-segment pointers → file offset
A full (already-uncompressed) virtual address in the tag segment is rebased:

```
fileOffset = virtualAddress - virtualBase + tagSegmentStartInFile
```

`virtualBase` comes from the header, `tagSegmentStartInFile` is derived from the
tag section's virtual address plus its offset mask (a signed delta). This is the
canonical "undo the memory mapping" step.

### 5.3 Debug-segment pointers → file offset
The debug segment never enters the engine's address space, so its pointers are
not rebased against a virtual base. Instead a fixed mask from the header is added
directly, and the result is truncated to 32 bits (the segment lives entirely
within 32-bit space and the arithmetic is designed to wrap correctly):

```
fileOffset = (uint32_t)(pointer + debugOffsetMask)
```

### 5.4 Resource-segment pointers → file offset
Resource block offsets are rebased with the resource section's own base address
and mask:

```
resourceOffset = blockOffset + resourceVirtualAddress + resourceOffsetMask
```

After this, page/segment indirection (and possible decompression / external-cache
redirection) yields the final bytes.

### 5.5 Compressed tag pointers → virtual address
There is a further wrinkle in the tag segment: the pointer a `TagEntry` stores to
its own metadata is **compressed**, not a full virtual address. At build time the
tools stored:

```
storedValue = (virtualAddress - segmentBase) >> 2
```

The right-shift by 2 (divide by 4) lets a full virtual address fit into a 32-bit
field without losing precision, because all tag addresses are 4-byte aligned. To
recover the address, the reader reverses it:

```
virtualAddress = (storedValue << 2) + ExpandMagic
```

`ExpandMagic` (`0x50000000`) is the segment base that was subtracted before
compression, it encodes where the tag segment was anchored in the engine's
address space at build time. Null (`0x0`) and sentinel (`0xFFFFFFFF`) values are
invalid and resolve to zero.

The output is still a *virtual address*, so it must then go through §5.2. The two
steps are always paired: **expand the compression, then rebase the address.**
Every compressed pointer in the tag segment (tag metadata pointers, tag-block
element pointers) goes through this pair.

---

## 6. The bootstrap chain (Header → tables)

The reader cannot touch any tag until it has walked a fixed chain out of the
header. Each step depends on the previous one, so the order is mandatory.

### 6.1 Header
A fixed `0xA000`-byte block at offset 0. The fields a reader must extract:

- **Virtual base address**: the anchor for all tag-segment translation.
- **Index header address**: a virtual pointer to the index header, rebased
  (§5.2) to find the root of the tag system.
- **Filename table fields**: count, data offset, size, and index offset of the
  debug segment's name tables.
- **Offset mask table** and **section table**: the four-entry arrays describing
  each segment's mask and bounds (§3). From the tag entries the reader derives
  where the tag segment begins in the file, from the index header address it
  derives the index header's file offset.

Virtual addresses in this format are 32-bit-meaningful even when stored as
uint64, the upper bits are discarded deliberately.

### 6.2 Index Header
The **root of the tag system**. It is not at a fixed offset, its position is the
(rebased) index header address from the header. It begins with a magic value,
`'tags'` (`0x74616773`), which a reader validates first to confirm both that the
file is intact and that the address arithmetic landed in the right place.

The index header then gives the **count and virtual address** of the two tables
that matter:

- **Tag group table**: the class system.
- **Tag table**: every tag instance.

Both addresses are rebased (§5.2) before reading.

### 6.3 Tag Group table
A flat contiguous array (one entry per group). Each entry holds the group's magic
and the magics of its parent and grandparent, encoding the three-level
inheritance hierarchy. The magic is a packed FourCC and is unpacked big-endian
into a 4-character string (`0x7363656E` → `"scen"`). This table is what lets the
reader ask, for any tag, "what *type* is this?"

### 6.4 Tag table
The **master index** of every tag in the map. Each entry holds only three things:

- **Group index**: which group (type) the tag is, an index into §6.3.
- **Datum salt**: a generation counter used to detect stale handles.
- **Memory address**: the *compressed* virtual pointer to the tag's metadata
  block.

Crucially, **the tag's data is not here**: only the pointer to it. To read a
tag's fields, the reader runs the memory address through §5.5 then §5.2 and seeks
to the result.

---

## 7. Handles and resolving a tag

Tags reference each other (and are referenced by datum handles) through a 32-bit
value with two fields:

- **lower 16 bits**: the tag table index,
- **upper 16 bits**: the salt.

Resolving a handle means: isolate index and salt, reject the sentinel
(`index == 0xFFFF && salt == 0xFFFF`), bounds-check the index against the tag
table, and verify the salt matches the table entry's stored salt (a mismatch
means a stale/invalid handle). Only then is the entry trustworthy.

There are two ways to reach a tag's metadata offset, both ending in the same
final translation (`expand → rebase`):

- **By index**: when the index is already in hand (from a handle or a tag
  reference). O(1): index the tag table directly. This is the route used while
  iterating the whole table and while following references.
- **By name**: when only the filename is known (e.g.
  `objects\vehicles\human\warthog\warthog`). O(n): scan the table comparing names
  until a match is found.

The only difference is *how the entry is located*, the address math afterward is
identical.

---

## 8. The Tag Metadata Block

Once a tag's metadata offset is known, its layout is defined entirely by its
group. An offsets definition (per group) names each field, its byte offset, and
its type. A metadata block is built from exactly **three kinds of data**, and
these three cover the entire readable surface of the format:

### 8.1 Flat fields
A `float`, `int`, `vec3`, color, range, etc. The trivial case: seek to
`base + fieldOffset`, read N bytes.

### 8.2 Tag References (`0x10` bytes)
A structure embedded in one tag that points to a **different tag**. It does not
contain data, it contains a datum handle (in its lower 16 bits, a tag table
index) identifying the target. Following a reference means reading that handle,
extracting the index, and resolving it (§7) to the target tag's metadata offset.
A null or sentinel handle means "no reference."

This is the mechanism for cross-tag links, e.g. a model (`hlmt`) referencing its
physics model (`phmo`), collision (`coll`), render model (`mode`), and animation
(`jmad`). Such child tags are reached *through* the reference, not by iterating
them directly.

### 8.3 Tag Blocks (`0xC` bytes)
The mechanism for **nested data and variable-length arrays**, the most common
non-trivial structure in the format. A tag block stores:

- an **entry count**, and
- a **compressed pointer** to the first element.

Reading it means: resolve the pointer (§5.5 → §5.2) to the first element's file
offset, then step through the array at a fixed stride
(`element[i] = first + i * entrySize`). Any time a tag holds a list of
sub-structures, collision shapes, BSP clusters, instance lists, parts, it is a
tag block, and tag blocks nest arbitrarily deep.

---

## 9. The full navigation chain

```
.map file (disk)
└─ Header (0x0, 0xA000 bytes)
   ├─ VirtualBaseAddress        -> anchor for tag-segment rebasing
   ├─ IndexHeaderAddress        -> rebase -> Index Header
   ├─ Filename table fields     -> debug-segment translate -> tag names
   └─ Section / mask tables     -> segment bounds + per-segment deltas
      └─ Index Header (validated by 'tags' magic)
         ├─ TagGroupTableAddress -> rebase -> Tag Group table (the type system)
         └─ TagTableAddress      -> rebase -> Tag table
            └─ TagEntry[i]
               ├─ GroupIndex      -> Tag Group table -> tag's type
               ├─ DatumSalt       -> handle validation
               └─ MemoryAddress (compressed)
                  └─ expand -> rebase -> Tag Metadata Block
                     ├─ Flat fields    -> seek + read
                     ├─ Tag Reference  -> resolve handle -> another tag's block
                     └─ Tag Block      -> resolve -> { count, first element }
                                        └─ element[i] = first + i * stride
                                           (elements may themselves hold
                                            references and nested blocks)
```

---

## 10. Reading a whole tag generically

Because a group fully defines a tag's layout, a tag of any type can be
reconstructed by the same generic procedure:

1. **Resolve the type.** From the group, obtain the raw struct layout for the
   tag's fixed-size portion.
2. **Read the header portion.** A single read at the metadata offset captures all
   flat fields and the *headers* of every embedded tag block (their counts and
   pointers).
3. **Expand nested data.** Walk every tag block, resolve it, read its elements,
   and recurse, each element may itself contain blocks and references.

After this, the entire tag, including all dynamically-sized children, is
resident in memory.

At the map level, this is driven by iterating the tag table once and dispatching
on group magic. Two patterns appear:

- **Tags reached directly** by their own magic, `vehi`, `bloc`, `sbsp`, `weap`,
  `proj`, `bipd`, `eqip`, `scen`, `scnr`, `ctrl`, `mach`, `zone`, `play`, `sldt`,
  `lbsp`. (Tags with no filename but a recognizable magic, such as `play`/`zone`,
  can be given synthetic names.)
- **Tags reached through a parent**, the `hlmt` family, where each model tag
  carries references to its `phmo`, `coll`, `mode`, and `jmad`. These are loaded
  by following references, never by direct iteration.

---

## 11. Reference: key offsets and structures (MCC U13)

| Concept | Struct / field | Offset | Size / type |
|---|---|---|---|
| Map header | `Map_Header` | 0x0 | 0xA000 |
| Virtual base | `MapHeader.VirtualBaseAddress` | 0x2E0 | uint64 (lo32) |
| Index header ptr | `MapHeader.IndexHeaderAddress` | 0x2E8 | uint64 (lo32) |
| Filename count | `MapHeader.FileTableCount` | 0x20 | int32 |
| Filename data off | `MapHeader.FileTableOffset` | 0x24 | int32 |
| Filename data size | `MapHeader.FileTableSize` | 0x28 | int32 |
| Filename index off | `MapHeader.FileIndexTableOffset` | 0x2C | int32 |
| Offset mask table | `MapHeader.OffsetMasks` | 0x4CC | 4 × 0x4 |
| Section table | `MapHeader.Sections` | 0x4DC | 4 × 0x8 |
| ↳ section v-addr | `Sections.VirtualAddress` | +0x0 | uint32 |
| ↳ section size | `Sections.Size` | +0x4 | uint32 |
| Index header | `Map_IndexTableHeader` | — | 0x4C |
| ↳ group count | `.NumberOfTagGroups` | 0x0 | int32 |
| ↳ group table addr | `.TagGroupTableAddress` | 0x8 | uint64 |
| ↳ tag count | `.NumberOfTags` | 0x10 | int32 |
| ↳ tag table addr | `.TagTableAddress` | 0x18 | uint64 |
| ↳ magic | `.Magic` | 0x48 | `'tags'` 0x74616773 |
| Tag group entry | `Map_TagTableGroupEntry` | — | 0x10 |
| ↳ magic | `.Magic` | 0x0 | int32 (FourCC) |
| ↳ parent magic | `.ParentMagic` | 0x4 | int32 |
| ↳ grandparent magic | `.GranParentMagic` | 0x8 | int32 |
| Tag entry | `Map_TagTableEntry` | — | 0x8 |
| ↳ group index | `.TagGroupIndex` | 0x0 | int16 |
| ↳ datum salt | `.DatumIndexSalt` | 0x2 | uint16 |
| ↳ memory address | `.MemoryAddress` | 0x4 | uint32 (compressed) |
| Tag reference | `Map_TagRef` | — | 0x10 |
| ↳ group magic | `.TagGroupMagic` | 0x0 | int32 |
| ↳ datum index | `.DatumIndex` | 0xC | uint32 (handle) |
| Tag block | `Map_TagBlock` | — | 0xC |
| ↳ entry count | `.EntryCount` | 0x0 | int32 |
| ↳ pointer | `.Pointer` | 0x4 | uint32 (compressed) |
| Data reference | `Map_DataRef` | — | 0x14 |
| ↳ size | `.Size` | 0x0 | int32 |
| ↳ pointer | `.Pointer` | 0xC | uint32 |

Constants: `ExpandMagic = 0x50000000`, index header magic `'tags' = 0x74616773`.
Section/mask index map: `[0]` debug, `[1]` resource, `[2]` tag, `[3]` unidentified.
All offsets are those of `CacheType.MccHaloReachU13`.

---

## 12. What lies beyond the metadata model

The three building blocks (flat fields, tag references, tag blocks) describe how
to read any *tag*. Two larger areas of the format sit on top of, or beside, this
model:

- **Resources** (resolved, in use): the resource segment and its page/segment
  indirection, compression, and external-cache redirection. This is the path the
  geometry reader follows to turn a tag's resource reference into actual vertex
  and index bytes. It is its own subsystem and is documented separately.
- **Localization and scripting** (not yet consumed): multilingual string tables,
  language ranges, locale index tables, and compiled script/global data exist in
  the format but are outside the current reader's scope. The one remaining
  unidentified segment most likely belongs here.