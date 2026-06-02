# SBSP geometry extraction - Halo Reach (MCC, Update 13)

Technical guide to extracting render geometry, the world-space triangles of
**clusters** and **instanced geometries**, from the SBSP/LBSP tags of Halo Reach
`.map` files in the Master Chief Collection (build U13 /
`CacheType.MccHaloReachU13`).

The purpose of this document is to capture everything needed to implement a mesh
reader (`System_MeshReader`) from scratch: where the geometry lives, how the
buffers are addressed and decoded, how vertices are transformed into world-space,
and how triangles are assembled. It assumes the reader already has a working
`.map` reader (tag/group resolution, address arithmetic, resource pages), see
`MapReader.md` for that foundation.

---

## Credits

All of the format's reverse engineering was verified against **Reclaimer**
(https://github.com/Gravemind2401/Reclaimer) by Gravemind2401, licensed under
GPL-3.0. The mesh-access theory, buffer layout, unstrip algorithm, and instance
transform handling were ported from Reclaimer's C# implementation to C++. Every
decision regarding offset, mask, and transform order was cross-checked against
Reclaimer's code (mainly `HaloReach/scenario_structure_bsp.cs`,
`HaloReach/HaloReachCommon.cs`, `Common/XmlVertexBuilder.cs`,
`Core/Geometry/GeometryExtensions.cs`, and `Core/Geometry/RealBounds3D.cs`).

The struct/field names in this document come from definitions auto-generated from
Assembly templates, so they differ from Reclaimer's, §11 gives the
correspondence.

---

## 1. Goal and scope

For each SBSP in a map, extract **all render triangles in world-space**, fully
**materialized**, every instance's triangles baked into their final positions,
not stored once as a linked duplicate plus a transform. The output feeds top-down
map rendering and a raycasting / BVH system.

Two clarifications of scope:

- This is **render** geometry, not **collision** geometry. Collision lives in
  separate SBSP blocks (planes, BSP3D, MOPP codes) in a different format. Render
  geometry includes things with no hitbox (bushes, decoration), filtering by real
  collision is a separate concern.
- Because instances are materialized rather than linked-duplicated, the triangle
  counts will not match a tool like Reclaimer/Blender that counts the compressed
  representation. This is expected, not an error.

---

## 2. Where the geometry lives

A SBSP's geometry is spread across four tags/tables, each holding a different
piece. None of them holds everything, extraction is a join across all four.

- **SBSP** (`scenario_structure_bsp`): the *structure*. Defines which sections
  are drawn as the static world (**clusters**), which objects are repeated with a
  transform (**instanced geometry instances**), and the **compression bounds** of
  each section. It does **not** contain vertex/index bytes.
- **LBSP** (`scenario_lightmap_bsp_data`): the *sections (meshes)*. Each section
  declares its vertex-buffer index, index-buffer index, vertex format, topology,
  and its **Parts** (submeshes). It points (via a resource datum) to where the
  bytes are.
- **Zone / TagResources**: the *resource table*. The section's datum resolves to
  a resource entry that carries the **ResourceFixups** (the in-page offsets of
  each VB/IB) and the **FixupData** (info-arrays with each buffer's size and
  vertex/format).
- **Play**: the *physical bytes*. Resolves a resource to actual pages and
  segments: where the bytes are (this map or an external cache like `shared.map`
  / `campaign.map`), whether they are compressed, and how to trim them.

Conceptual resolution for one section:

```
datum -> TagResources[idx] -> Play.Segments[seg] -> Play.RawPages[page]
      -> (decompress, trim to segment) -> pageData (flat byte buffer)
ResourceFixups[VBidx]              -> offset of the VB within pageData
ResourceFixups[vbCount*2 + IBidx]  -> offset of the IB within pageData
FixupData info-arrays              -> DataLength + VertexCount of each buffer
```

The whole LBSP of one SBSP resolves to a **single resource page** (`pageData`)
that contains all of that SBSP's vertex and index buffers. The fixups index into
this one buffer.

---

## 3. The extraction pipeline (top-level order)

Implementing the reader is implementing this sequence, per SBSP. The order is
fixed because each step consumes the previous one's output.

```
for each SBSP:
  1. Resolve the LBSP resource page -> pageData + the LBSP resource entry
  2. Read the FixupData footer       -> vbCount, ibCount
  3. Read the FixupData info-arrays  -> per-buffer DataLength + VertexCount/format
  4a. CLUSTERS:  for each cluster, emit its section raw (no transform, no bounds)
  4b. INSTANCES: read the transform table, for each instance, expand bounds and
                 bake the transform, then emit its section
```

Steps 1–3 build the shared context (the page and the buffer metadata) used by
both 4a and 4b. The actual triangle assembly (§7–§9) is the same routine for
clusters and instances, they differ only in *whether* a transform and bounds are
applied.

---

## 4. Resolving the resource page (datum → bytes)

`datum & 0xFFFF` indexes `zone->TagResources`. The entry yields a
`PlaySegmentIndex` into `play->Segments`, and the segment selects a **page** with
a **segment offset** into it. Page selection follows the engine's "Auto" rule:

- Use the **secondary** page if `SecondaryPageIndex >= 0` (with its secondary
  segment offset), otherwise use the **primary**.
- If the chosen page is empty (`BlockOffset == 0xFFFFFFFF` or
  `CompressedBlockSize == 0`), **fall back to the primary** page and its offset.

Once a valid `RawPages` entry is chosen:

- If `SharedCacheIndex >= 0`, the bytes live in **another `.map`**. Resolve the
  path via `ExternalCacheReferences[idx].MapPath` (mapping `shared` → shared
  cache, `campaign` → campaign cache, others unsupported) and read from that
  file. If the path can't be resolved, skip the section.
- Otherwise the bytes are in this map.

In both cases the file position is `ToResourceOffset(page->BlockOffset)`, the
trimmed length is `UncompressedBlockSize - segmentOffset` (skip if `<= 0`), and
the page is decompressed (raw deflate, uncompressed if compressed size ==
uncompressed size) and trimmed to the segment. The result is `pageData`: the flat
buffer the fixup offsets index directly.

> The external (shared/campaign) cache only supplies **geometry bytes**.
> Instances and their transforms are always **local** to the map's own SBSP,
> there is no inheriting instances from an external cache.

---

## 5. FixupData: footer and info-arrays

The resource entry's fixup block ends with a **24-byte footer**, read relative to
the fixup data base:

```
footerPos = fixupDataBase + FixupInformationOffset + FixupInformationLength - 24
vbCount @ footer+0   (int32)
ibCount @ footer+12  (int32)
```

where `fixupDataBase = ToFileOffset(Expand(zone->FixupInformation.Pointer))`.

From `fixupDataBase + FixupInformationOffset` come three consecutive
**info-arrays**:

```
[ VertexBufferInfo  x vbCount ]   28 bytes each: VertexCount @0, DataLength @8
[ aux               x vbCount ]   12 bytes each  (skipped)
[ IndexBufferInfo   x ibCount ]   28 bytes each: IndexFormat  @0, DataLength @8
```

Two fields matter downstream:

- **VertexBufferInfo.VertexCount** is required, it is what derives the real
  vertex stride (§6). It is not discardable.
- **VertexBufferInfo.DataLength** / **IndexBufferInfo.DataLength** give the byte
  length of each buffer, used both for stride derivation and for bounds-checking
  reads against `pageData`.

The IndexBufferInfo's `IndexFormat` is **not** reliable for deciding topology,
use the LBSP field instead (§8).

---

## 6. Vertex stride and position

The vertex **position is Float32 @ offset 0** of the vertex in all relevant MCC
formats (`s_world_vertex` 0x00, `s_rigid_vertex` 0x01, `s_skinned_vertex` 0x02,
`s_decorator_vertex` 0x0F uses Float32_3). What varies per section is the
**stride**:

```
World/Rigid -> 0x24   Skinned -> 0x2C   Decorator -> 0x20
```

Rather than keep a table keyed by `VertexType`, derive the stride directly from
the buffer, self-correcting for any format:

```
stride = VertexBufferInfo.DataLength / VertexBufferInfo.VertexCount
```

with a fallback of `0x24` when `VertexCount` is 0 or does not divide
`DataLength` evenly (log a warning in that case, it signals an unexpected
format). Then `vertexCount = DataLength / stride`. The **index width** is 16-bit
unless `vertexCount > 65535`, in which case it is 32-bit.

Only `VertexBufferIndex1` (the section's primary VB) is used for position
geometry.

---

## 7. Vertex transform: bounds expansion, then instance matrix

Each vertex is decoded, then transformed in a fixed order: **expand bounds
first, then apply the instance matrix.**

### 7.1 Bounds expansion (de-normalization)
Instance vertex positions are **normalized to [0,1]** and must be expanded with
the section's bounds, cluster vertices are already raw world-space and skip this.
The expansion is per-axis:

```
world = Min + raw * (Max - Min)        // per axis X, Y, Z
```

This mirrors Reclaimer's `RealBounds3D.CreateExpansionMatrix` (scale on the
diagonal = range length, translation = minimum). The discriminant for "expand or
not" is **whether bounds are empty**: if `Min == Max` on all three axes (or there
are no bounds for the section), the expansion is identity and the position is
read raw. This is exactly how clusters end up read raw, they have no bounds (§9).
Note this follows Reclaimer in using `Min == Max` as the identity condition,
**not** a `CompressionFlags` check.

### 7.2 Instance matrix
For instances, after expansion the local position is baked by the instance's 3×4
row-major matrix (rotation/scale in `m[0..8]`, translation in `m[9..11]`):

```
wx = x*m[0] + y*m[3] + z*m[6] + m[9]
wy = x*m[1] + y*m[4] + z*m[7] + m[10]
wz = x*m[2] + y*m[5] + z*m[8] + m[11]
```

Clusters pass no matrix and keep the (already world-space) position unchanged.

---

## 8. Topology: list vs strip

The reliable topology field is **`Lbsp_MeshesEntry.IndexBufferType` (@0x32)** of
the LBSP, *not* the FixupData's `IndexFormat`, and *not* a `0xFFFF` restart-index
heuristic (Reach does not use restart indices).

```
IndexBufferType == 3        -> Triangle List   (consume i, i+1, i+2, step 3)
IndexBufferType == 5 or 0   -> Triangle Strip  -> unstrip
```

The **unstrip** (matching Reclaimer's `GeometryExtensions.Unstrip`) does not rely
on an explicit restart index. It slides a 3-index window, detects restarts by
**degenerate triangles** (any two indices equal), and alternates winding by
parity:

```
window (i0,i1,i2); for each new index along the range:
  i0=i1; i1=i2; i2=new
  pos++                                   // count of indices consumed
  if pos < 3: continue                    // window not full yet
  if i0==i1 || i0==i2 || i1==i2: continue // degenerate -> skip (restart)
  if pos odd:  emit (i0,i1,i2)
  if pos even: emit (i0,i2,i1)            // flip winding
```

In practice Reach U13 SBSPs are almost entirely Triangle List, but reading the
correct field removes the guesswork.

### Unindexed case
If the section has no index buffer (`IndexBufferIndex < 0`) or the `MeshFlags`
bit 4 ("Mesh Is Unindexed") is set, the IB is implicit (0..N-1 over the VB) and
is interpreted as an implicit strip with alternating winding.

---

## 9. Emitting a section

The triangle-assembly routine is shared by clusters and instances. Given the
page, the section, its (optional) bounds, and its (optional) instance matrix:

1. **Resolve the VB.** `VertexBufferIndex1` indexes both the info-arrays and the
   `ResourceFixups`. The VB offset is `MaskedOffset(ResourceFixups[vbIdx])`
   (§10), its length is `VertexBufferInfo[vbIdx].DataLength`. Bounds-check
   `vbOff + vbLen <= pageData.size()` before reading.
2. **Derive stride and vertexCount** (§6).
3. **Set up bounds** (§7.1), identity if none.
4. **Decode the IB.** Unindexed → implicit strip (§8). Otherwise the IB lives at
   fixup slot `vbCount*2 + IndexBufferIndex`, its offset is `MaskedOffset` of that
   slot and its length is `IndexBufferInfo[ibIdx].DataLength`. Bounds-check
   against `pageData`. Index width follows §6.
5. **Iterate by Parts.** Emit only the range
   `[Part.IndexStart, Part.IndexStart + Part.IndexCount)` of each Part, decoding
   each range as list or strip (§8). If the section has no Parts, fall back to the
   whole IB. Reading the full IB blindly drags inter-Part padding in as garbage
   triangles, so Parts are the correct granularity. (Subparts are visibility
   subdivisions of Parts, not extra triangles, ignore them for extraction.)
6. **Per triangle**, read each index's vertex (position @0 → expand → matrix) and
   reject the triangle if any two indices are equal or any index is
   `>= vertexCount`. This guards against degenerates and malformed ranges.

Defensive bounds-checking at every read (VB offset, IB offset, index value,
vertex index) is part of the spec, not optional: a single bad fixup or count
otherwise reads from the wrong address and produces formless geometry or a crash.

---

## 10. The 28-bit fixup offset mask

Each `ResourceFixupsEntry` encodes a buffer's offset within `pageData` across
three fields: `Address` (uint16 @4), `AddressUpperBits` (uint8 @6),
`AddressLocationHighBits` (uint8 @7). Reconstruct the full **28-bit** offset and
mask with `0x0FFFFFFF`:

```
offset = Address | (AddressUpperBits << 16) | (AddressLocationHighBits << 24)
offset &= 0x0FFFFFFF
```

All three bytes are required. Dropping `AddressLocationHighBits` (a 24-bit
reconstruction) is invisible for any buffer below 16 MB but truncates the offset
of any buffer past 16 MB on large pages, reading from the wrong address. Use the
full 28 bits everywhere a fixup offset is read.

### Instance transform table address
The instance transform table is reached through the **instances** resource entry
(`SbspData.ZoneAssetDatum5`, "InstancesResourcePointer"), using a specific fixup
slot and applying the mask to the **full sum** of the fixup offset and the raw
offset (as Reclaimer does):

```
fx        = instEntry->ResourceFixups[ Count - 10 ]
rawOffset = (fx.AddressUpperBits << 16) | fx.Address
address   = (instEntry->FixupInformationOffset + rawOffset) & 0x0FFFFFFF
base      = fixupDataBase + address
```

(Guard that `Count - 10 >= 0`.) From `base`, the instances are a packed array of
**156-byte** blocks, one per `InstancedGeometryInstances` entry:

```
@0    float    TransformScale       // uniform scale, applied to m[0..8] only
@4    48 bytes Matrix3x4            // 12 floats row-major (rot/scale [0..8], transl [9..11])
@52   6 bytes  padding
@58   int16    SectionIndex         // index into lbsp->Meshes
...
@154  int16    (unrelated, NOT the SectionIndex)
```

For each instance: read scale and matrix, multiply `m[0..8]` by scale (rotation/
scale only, never translation), read `SectionIndex` at **@58**, look up
`lbsp->Meshes[SectionIndex]`, take its bounds from `CompressionInfo_3[SectionIndex]`
(§7.1), and emit (§9) with that matrix.

---

## 11. Bounds source, clusters, and the three compression groups

The SBSP carries **three** parallel geometry/compression groups, exposed in the
struct as `CompressionInfo`, `CompressionInfo_2`, `CompressionInfo_3`. The
**correct bounds are in `CompressionInfo_3`** (Reclaimer's `BoundingBoxes` at
SBSP offset 1116 = 0x45C for U13). In some maps (e.g. Forge World) the first two
groups are empty and only the third carries real bounds, reading the first group
yields no expansion and collapses instances into a unit cube.

The mapping is positional: `CompressionInfo_3[i]` describes the bounds of
`Meshes[i]`, for `i` up to the group's count. Sections **above** that count are
the **World clusters**, which receive no bounds and are read raw (their positions
are already world-space Float32, empirically, a cluster's first vertex is real
world coordinates in the hundreds, not normalized).

Accordingly:

- **Clusters.** Each `sbsp->Clusters[i].MeshIndex` (@0x40) selects
  `lbsp->Meshes[MeshIndex]`. Emit it with **no transform** and **no bounds**.
- **Instances.** Each instance selects its section via the 156-byte block's
  `SectionIndex` (§10), expands with `CompressionInfo_3[SectionIndex]`, and bakes
  its matrix.

---

## 12. Name mapping (Assembly ↔ Reclaimer)

| Concept | Struct/field (Assembly) | Offset | Reclaimer |
|---|---|---|---|
| Geometry section | `Lbsp_MeshesEntry` | — | `SectionBlock` |
| Section's VB | `.VertexBufferIndex1` | 0x18 | `VertexBufferIndex` |
| Section's IB | `.IndexBufferIndex` | 0x28 | `IndexBufferIndex` |
| Mesh flags | `.MeshFlags` | 0x2C | `Flags` |
| Vertex format | `.VertexType` | 0x2F | `VertexFormat` |
| Topology | `.IndexBufferType` | 0x32 | `IndexFormat` |
| Submesh start | `Lbsp_Meshes_PartsEntry.IndexStart` | 0x4 | `Submesh.IndexStart` |
| Submesh count | `Lbsp_Meshes_PartsEntry.IndexCount` | 0x8 | `Submesh.IndexLength` |
| Cluster → section | `Sbsp_ClustersEntry.MeshIndex` | 0x40 | `Cluster.SectionIndex` |
| Section bounds | `Sbsp_CompressionInfoEntry_3` | @1116 | `BoundingBoxBlock` |
| ↳ X/Y/Z bounds | `.PositionBounds{X,Y,Z}` | 0x4/0xC/0x14 | `XBounds/YBounds/ZBounds` |
| Instances ptr | `SbspData.ZoneAssetDatum5` | 0x520 | `InstancesResourcePointer` |
| LBSP geometry ptr | `LbspData.ZoneAssetDatum` | 0x10C | `ResourcePointer` |
| Fixup offset entry | `Zone_TagResourcesEntry.FixupInformationOffset` | 0x14 | `FixupOffset` |
| Fixup length entry | `.FixupInformationLength` | 0x18 | `FixupSize` |
| Play segment | `.PlaySegmentIndex` | 0x22 | — |

Constants: fixup mask `0x0FFFFFFF` (28-bit), VB stride fallback `0x24`, instance
stride 156 bytes, instance fixup slot `Count - 10`, IB fixup slot
`vbCount*2 + IndexBufferIndex`. `CompressionInfo_3` is at SBSP offset 1116 for U13
(Reclaimer's offset differs per build: Beta/Retail/MCC/U13 = 1124/1112/1140/1116).
All offsets are those of `CacheType.MccHaloReachU13`.