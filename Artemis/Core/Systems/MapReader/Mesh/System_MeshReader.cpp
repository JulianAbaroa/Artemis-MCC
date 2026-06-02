// This geometry extraction logic is derived from Reclaimer
//   (https://github.com/Gravemind2401/Reclaimer) by Gravemind2401,
//   licensed under GPL-3.0. The mesh-access theory, buffer layout,
//   unstrip algorithm and instance transform handling were ported
//   from Reclaimer's C# implementation to C++.
// See MeshReader.md for the full Assembly <-> Reclaimer name mapping.

#include "pch.h"

#include "System_MeshReader.h"

#include "Core/Types/Map/MapMagics.h"
#include "Core/Types/Navigation/SbspGeometry.h"

#include "Core/States/MapReader/State_MapReader.h"
#include "Core/States/MapReader/Scnr/State_MapScnr.h"
#include "Core/States/MapReader/Sbsp/State_MapSbsp.h"
#include "Core/States/MapReader/Lbsp/State_MapLbsp.h"
#include "Core/States/MapReader/Play/State_MapPlay.h"
#include "Core/States/MapReader/Zone/State_MapZone.h"

#include "../System_MapReader.h"
#include "Core/Systems/Logs/System_Logs.h"

// Extracts the render triangles of each SBSP in world-space:
//   - Clusters World: raw world-space, without transform or bounds.
//   - Instanced geometries Rigid: normalized position [0,1] expanded by
//     bounds (CompressionInfo_3) and then baked with its 3x4 transform.
//
// Verificated against Reclaimer (github.com/Gravemind2401/Reclaimer) 
// and against real dumps from campaign (m20 & forge_halo).
bool System_MeshReader::ReadSbspGeometries(
    std::vector<SbspGeometry>& outGeometries)
{
    const ZoneObject* zone = m_Deps.State_MapZone.GetFirstZone();
    const PlayObject* play = m_Deps.State_MapPlay.GetFirstPlay();
    if (!zone || !play) return false;

    // World/Rigid Float32_4 @0.
    const uint32_t VB_STRIDE_FALLBACK = 0x24;   
    const uint32_t FIXUP_MASK = 0x0FFFFFFF;

    // Buffer offset within pageData. 28-bit mask (4 bytes @4..7).
    auto MaskedOffset = [&](const Zone_TagResources_ResourceFixupsEntry& fx) -> uint32_t {
        uint32_t off = (uint32_t)fx.Address | 
            ((uint32_t)fx.AddressUpperBits << 16) | 
            ((uint32_t)fx.AddressLocationHighBits << 24);
        return off & FIXUP_MASK;
    };

    const int64_t fixupDataBase = m_Deps.System_MapReader.ToFileOffset(
        m_Deps.System_MapReader.Expand(zone->Data.FixupInformation.Pointer));

    // Footer of the entry block: vbCount @+0, ibCount @+12 (24B from the end).
    auto ReadFooter = [&](const Zone_TagResourcesObject& entry,
        int32_t& outVbCount, int32_t& outIbCount) -> bool {
            std::string filePath = m_Deps.State_MapReader.GetMapFilePath();

            int64_t footerPos = fixupDataBase + entry.FixupInformationOffset
                + entry.FixupInformationLength - 24;

            auto buf = m_Deps.System_MapReader.ReadDataFrom(
                filePath, footerPos, 24);

            if (buf.size() < 24) return false;

            memcpy(&outVbCount, buf.data() + 0, 4);
            memcpy(&outIbCount, buf.data() + 12, 4);

            return true;
        };

    // FixupData Info-arrays:
    //   [ VertexBufferInfo x vbCount ] (28B, VertexCount@0, DataLength@8)
    //   [ aux 12B          x vbCount ] (skips)
    //   [ IndexBufferInfo  x ibCount ] (28B, IndexFormat@0, DataLength@8)
    // Aux = VertexCount(VB), used to derive stride.
    struct BufferInfo 
    { 
        uint32_t DataLength; 
        uint32_t Aux; 
    };

    auto ReadInfoArrays = [&](const Zone_TagResourcesObject& entry,
        int32_t vbCount, int32_t ibCount, std::vector<BufferInfo>& outVbInfo,
        std::vector<BufferInfo>& outIbInfo) -> bool {
            const int32_t kInfoStride = 28;
            const int32_t kAuxStride = 12;

            int64_t blockBase = fixupDataBase + entry.FixupInformationOffset;

            int64_t vbInfoBytes = (int64_t)vbCount * kInfoStride;
            int64_t auxBytes = (int64_t)vbCount * kAuxStride;
            int64_t ibInfoBytes = (int64_t)ibCount * kInfoStride;

            int64_t total = vbInfoBytes + auxBytes + ibInfoBytes;
            if (total <= 0) return false;

            std::string filePath = m_Deps.State_MapReader.GetMapFilePath();

            auto buf = m_Deps.System_MapReader.ReadDataFrom(
                filePath, blockBase, (int32_t)total);

            if ((int64_t)buf.size() < total) return false;

            outVbInfo.resize(vbCount);
            for (int i = 0; i < vbCount; ++i)
            {
                const uint8_t* p = buf.data() + (int64_t)i * kInfoStride;
                uint32_t vcount = 0, dlen = 0;
                memcpy(&vcount, p + 0, 4);
                memcpy(&dlen, p + 8, 4);
                outVbInfo[i] = { dlen, vcount };
            }

            outIbInfo.resize(ibCount);
            int64_t ibBase = vbInfoBytes + auxBytes;

            for (int i = 0; i < ibCount; ++i)
            {
                const uint8_t* p = buf.data() + ibBase + 
                    (int64_t)i * kInfoStride;

                uint32_t fmt = 0, dlen = 0;
                memcpy(&fmt, p + 0, 4);
                memcpy(&dlen, p + 8, 4);
                outIbInfo[i] = { dlen, fmt };
            }

            return true;
        };

    // Resolve a resource (datum) -> uncompressed buffer trimmed to the segment.
    auto ReadResourcePage = [&](uint32_t datum, 
        const Zone_TagResourcesObject** outEntry) -> std::vector<uint8_t> {
        int resIdx = (int)(datum & 0xFFFF);
        if (resIdx < 0 || resIdx >= (int)zone->TagResources.size())
        {
            return {};
        }

        const Zone_TagResourcesObject& entry = zone->TagResources[resIdx];
        if (outEntry) *outEntry = &entry;

        int segIdx = (int)entry.PlaySegmentIndex;
        if (segIdx < 0 || segIdx >= (int)play->Segments.size()) return {};
        const Play_SegmentsEntry& seg = play->Segments[segIdx];

        // PageType.Auto: secondary if it exists, with fallback to primary.
        int pageIdx = (int)(int16_t)seg.PrimaryPageIndex;
        int32_t segOffset = seg.PrimarySegmentOffset;

        if ((int16_t)seg.SecondaryPageIndex >= 0)
        {
            pageIdx = (int)(int16_t)seg.SecondaryPageIndex;
            segOffset = seg.SecondarySegmentOffset;
        }

        if (pageIdx < 0 || pageIdx >= (int)play->RawPages.size())
        {
            return {};
        }

        const Play_RawPagesEntry* page = &play->RawPages[pageIdx];

        // Fallback to primary if secondary school is empty.
        if (page->BlockOffset == 0xFFFFFFFFu || 
            page->CompressedBlockSize == 0)
        {
            pageIdx = (int)(int16_t)seg.PrimaryPageIndex;
            segOffset = seg.PrimarySegmentOffset;

            if (pageIdx < 0 || pageIdx >= (int)play->RawPages.size())
            {
                return {};
            }

            page = &play->RawPages[pageIdx];
        }

        // External cached page (shared/campaign/english/mainmenu).
        if (page->SharedCacheIndex >= 0)
        {
            std::string extPath;

            int extIdx = (int)page->SharedCacheIndex;
            if (extIdx < (int)play->ExternalCacheReferences.size())
            {
                const char* mapPath = reinterpret_cast<const char*>(
                    &play->ExternalCacheReferences[extIdx].MapPath);

                extPath = m_Deps.System_MapReader.
                    ResolveExternalCachePath(mapPath);
            }

            if (extPath.empty())
            {
                m_Deps.System_Logs.Log("[MeshReader] WARNING: Resource"
                    " External cache resource without a resolvable path,"
                    " omitting. (idx=%d)", extIdx);
                return {};
            }

            int64_t filePosExt = m_Deps.System_MapReader.ToResourceOffset(
                (int64_t)page->BlockOffset);

            int32_t segLenExt = 
                (int32_t)page->UncompressedBlockSize - segOffset;
            if (segLenExt <= 0) return {};

            return m_Deps.System_MapReader.ReadResourceDataDecompressedFrom(
                extPath, filePosExt, (int32_t)page->CompressedBlockSize,
                (int32_t)page->UncompressedBlockSize, segOffset, segLenExt);
        }

        int64_t filePos = m_Deps.System_MapReader.ToResourceOffset(
            (int64_t)page->BlockOffset);

        int32_t segLen = 
            (int32_t)page->UncompressedBlockSize - segOffset;
        if (segLen <= 0) return {};

        return m_Deps.System_MapReader.ReadResourceDataDecompressed(
            filePos, (int32_t)page->CompressedBlockSize,
            (int32_t)page->UncompressedBlockSize, segOffset, segLen);
    };

    // Emit the triangles of one section towards 'out'.
    //   comp: bounds of the section (nullptr or Min==Max -> raw reading).
    //   xform: 3x4 row-major instance matrix (nullptr for clusters).
    auto EmitSection = [&](const std::vector<uint8_t>& pageData,
        const Zone_TagResourcesObject& entry, int32_t vbCount,
        const std::vector<BufferInfo>& vbInfo,
        const std::vector<BufferInfo>& ibInfo,
        const Lbsp_MeshesObject& sec,
        const Sbsp_CompressionInfoEntry_3* comp,
        const char* tagNameForLog, const float* xform,
        std::vector<SbspTriangle>& out) -> uint32_t {
        int vbIdx = sec.VertexBufferIndex1;
        int ibIdx = sec.IndexBufferIndex;
        if (vbIdx < 0 || vbIdx >= vbCount) return 0;
        if (vbIdx >= (int)vbInfo.size()) return 0;
        if (vbIdx >= (int)entry.ResourceFixups.size()) return 0;
        
        // --- Vertex buffer ---
        uint32_t vbOff = MaskedOffset(entry.ResourceFixups[vbIdx]);
        uint32_t vbLen = vbInfo[vbIdx].DataLength;
        if ((int64_t)vbOff + vbLen > (int64_t)pageData.size()) return 0;
        const uint8_t* vb = pageData.data() + vbOff;
        
        // Stride per section: DataLength / VertexCount (fallback 0x24).
        uint32_t vCountInfo = vbInfo[vbIdx].Aux;
        uint32_t stride = VB_STRIDE_FALLBACK;
        if (vCountInfo > 0 && (vbLen % vCountInfo) == 0)
        {
            stride = vbLen / vCountInfo;
        }
        else if (vCountInfo > 0)
        {
            m_Deps.System_Logs.Log("[MeshReader] WARNING: '%s': VB %d DataLength=%u"
                " not divisible by VertexCount=%u, stride fallback 0x%X "
                " (VertexType=%u).", tagNameForLog ? tagNameForLog : "?", 
                vbIdx, vbLen, vCountInfo, VB_STRIDE_FALLBACK, 
                (unsigned)sec.VertexType);
        }

        if (stride == 0) return 0;
        uint32_t vertexCount = vbLen / stride;
        if (vertexCount == 0) return 0;
        
        // Positional bounds. Reclaimer ignores CompressionFlags: expands if
        // bounds are not empty(Min != Max). Normalized instances[0, 1],
        // raw clusters(no bounds->identity).
        bool hasBounds = comp != nullptr && 
            !(comp->PositionBoundsX.Min == comp->PositionBoundsX.Max && 
            comp->PositionBoundsY.Min == comp->PositionBoundsY.Max && 
            comp->PositionBoundsZ.Min == comp->PositionBoundsZ.Max);

        float minX = 0, minY = 0, minZ = 0, lenX = 1, lenY = 1, lenZ = 1;

        if (hasBounds)
        {
            minX = comp->PositionBoundsX.Min; 
            lenX = comp->PositionBoundsX.Max - comp->PositionBoundsX.Min;

            minY = comp->PositionBoundsY.Min; 
            lenY = comp->PositionBoundsY.Max - comp->PositionBoundsY.Min;

            minZ = comp->PositionBoundsZ.Min; 
            lenZ = comp->PositionBoundsZ.Max - comp->PositionBoundsZ.Min;
        }
        
        // Read a vertex: position Float32 @0 -> expand bounds -> xform.
        auto readVtx = [&](uint32_t idx) -> SbspVec3 {
            if (idx >= vertexCount) return { 0, 0, 0 };

            const uint8_t* p = vb + (size_t)idx * stride;
            float x, y, z;
            memcpy(&x, p + 0, 4);
            memcpy(&y, p + 4, 4);
            memcpy(&z, p + 8, 4);
        
            if (hasBounds)
            {
                x = minX + x * lenX;
                y = minY + y * lenY;
                z = minZ + z * lenZ;
            }

            if (xform)
            {
                float wx = x * xform[0] + y * xform[3] + 
                    z * xform[6] + xform[9];

                float wy = x * xform[1] + y * xform[4] + 
                    z * xform[7] + xform[10];

                float wz = x * xform[2] + y * xform[5] + 
                    z * xform[8] + xform[11];

                return { wx, wy, wz };
            }

            return { x, y, z };
        };
        
        uint32_t emitted = 0;

        auto pushTri = [&](uint32_t a, uint32_t b, uint32_t c) {
            if (a == b || b == c || a == c) return;
            if (a >= vertexCount || b >= vertexCount || c >= vertexCount)
            {
                return;
            }

            SbspTriangle t;
            t.V0 = readVtx(a);
            t.V1 = readVtx(b);
            t.V2 = readVtx(c);
            out.push_back(t);
            ++emitted;
        };
        
        // Unindexed case: implicit strip 0..N-1 over the VB.
        bool unindexed = (ibIdx < 0) || (sec.MeshFlags & (1u << 4));
        if (unindexed)
        {
            for (uint32_t i = 0; i + 2 < vertexCount; ++i)
            {
                if (i & 1) pushTri(i, i + 2, i + 1);
                else pushTri(i, i + 1, i + 2);
            }

            return emitted;
        }
        
        if (ibIdx >= (int)ibInfo.size()) return emitted;
        
        // Index buffer: ResourceFixups[ vbCount*2 + IndexBufferIndex ].
        int fxIb = vbCount * 2 + ibIdx;
        if (fxIb < 0 || fxIb >= (int)entry.ResourceFixups.size())
        {
            return emitted;
        }

        uint32_t ibOff = MaskedOffset(entry.ResourceFixups[fxIb]);
        uint32_t ibLen = ibInfo[ibIdx].DataLength;
        if ((int64_t)ibOff + ibLen > (int64_t)pageData.size())
        {
            return emitted;
        }
        
        bool wide = (vertexCount > 65535);
        uint32_t idxStride = wide ? 4u : 2u;
        uint32_t idxCount = ibLen / idxStride;
        const uint8_t* ibp = pageData.data() + ibOff;
        
        auto readIdx = [&](uint32_t i) -> uint32_t {
            if (wide) 
            { 
                uint32_t v; 
                memcpy(&v, ibp + (size_t)i * 4, 4); 
                return v; 
            }

            uint16_t v;
            memcpy(&v, ibp + (size_t)i * 2, 2); 
            return v;
        };
        
        // Topology by IndexBufferType @0x32. 3=List; 5/0=Strip(unstrip).
        bool isStrip = (sec.IndexBufferType == 5) || 
            (sec.IndexBufferType == 0);

        // Processes a range [start,end) from the IB as a list or strip.
        auto emitRange = [&](uint32_t start, uint32_t count) {
            if (count == 0) return;

            uint64_t end = (uint64_t)start + count;
            if (end > idxCount) end = idxCount;
            if (start >= end) return;
        
            if (!isStrip)
            {
                for (uint64_t i = start; i + 2 < end; i += 3)
                {
                    pushTri(readIdx((uint32_t)i), readIdx((uint32_t)i + 1),
                        readIdx((uint32_t)i + 2));
                }

                return;
            }
        
            // Unstrip like Reclaimer: degenerate reboot, alternate winding.
            uint32_t i0 = 0, i1 = 0, i2 = 0;
            uint64_t pos = 0;

            for (uint64_t i = start; i < end; ++i)
            {
                uint32_t idx = readIdx((uint32_t)i);
                i0 = i1; i1 = i2; i2 = idx;

                if (pos++ < 2) continue;
                if (i0 == i1 || i0 == i2 || i1 == i2) continue;

                if (pos & 1) pushTri(i0, i1, i2);
                else pushTri(i0, i2, i1);
            }
        };
        
        // Iteration by Parts (submeshes). Fallback: Complete IB.
        if (!sec.Parts.empty())
        {
            for (const auto& part : sec.Parts)
            {
                emitRange(part.IndexStart, part.IndexCount);
            }
        }
        else
        {
            emitRange(0, idxCount);
        }
        
        return emitted;
    };

    // Main loop for SBSP.
    int32_t sbspsProcessed = 0;

    for (SbspGeometry& geo : outGeometries)
    {
        const SbspObject* sbsp = m_Deps.State_MapSbsp.GetSbsp(geo.TagName);
        if (!sbsp) continue;

        const LbspObject* lbsp = m_Deps.State_MapLbsp.GetLbsp(geo.TagName);
        if (!lbsp || lbsp->Meshes.empty()) continue;

        // (1) Page + entry of the LBSP (contains all sections).
        const Zone_TagResourcesObject* lbspEntry = nullptr;

        std::vector<uint8_t> pageData =
            ReadResourcePage(lbsp->Data.ZoneAssetDatum, &lbspEntry);

        if (pageData.empty() || !lbspEntry)
        {
            m_Deps.System_Logs.Log("[MeshReader] INFO: '%s': "
                " LBSP page empty/omitted", geo.TagName.c_str());
            continue;
        }

        // (2) Counts + info-arrays of the FixupData.
        int32_t vbCount = 0, ibCount = 0;
        if (!ReadFooter(*lbspEntry, vbCount, ibCount) || vbCount <= 0)
        {
            m_Deps.System_Logs.Log("[MeshReader] WARNING: '%s':"
                " invalid footer", geo.TagName.c_str());
            continue;
        }

        std::vector<BufferInfo> vbInfo, ibInfo;
        if (!ReadInfoArrays(*lbspEntry, vbCount, ibCount, vbInfo, ibInfo))
        {
            m_Deps.System_Logs.Log("[MeshReader] WARNING: '%s': "
                " invalid info-arrays", geo.TagName.c_str());
            continue;
        }

        // Bounds per section. 
        // CompressionInfo_3 == Reclaimer BoundingBoxes @1116.
        // Parallel 1:1 to Meshes, sections above their size 
        // (clusters World) -> nullptr -> raw read.
        auto CompFor = [&](int secIdx) -> const Sbsp_CompressionInfoEntry_3* {
            if (secIdx < 0 || secIdx >= (int)sbsp->CompressionInfo_3.size())
            {
                return nullptr;
            }

            return &sbsp->CompressionInfo_3[secIdx];
        };

        // (3) CLUSTERS: section by cluster.MeshIndex, raw world-space.
        uint32_t triWorld = 0;

        for (const auto& cluster : sbsp->Clusters)
        {
            int secIdx = cluster.MeshIndex;
            if (secIdx < 0 || secIdx >= (int)lbsp->Meshes.size()) continue;

            const Lbsp_MeshesObject& sec = lbsp->Meshes[secIdx];

            triWorld += EmitSection(pageData, *lbspEntry, vbCount,
                vbInfo, ibInfo, sec, nullptr, geo.TagName.c_str(),
                nullptr, geo.RenderMesh);
        }

        // (4) INSTANCES: transform baking + bounds expansion.
        uint32_t triInst = 0;

        if (!sbsp->InstancedGeometryInstances.empty())
        {
            const Zone_TagResourcesObject* instEntry = nullptr;

            int instResIdx = (int)(sbsp->Data.ZoneAssetDatum5 & 0xFFFF);
            if (instResIdx >= 0 && instResIdx < (int)zone->TagResources.size())
            {
                instEntry = &zone->TagResources[instResIdx];
            }

            if (instEntry)
            {
                int fxN = (int)instEntry->ResourceFixups.size() - 10;

                if (fxN >= 0 && fxN < (int)instEntry->ResourceFixups.size())
                {
                    // base = fixupDataBase + ((FixupOffset + rawOffset) & MASK).
                    const auto& fx = instEntry->ResourceFixups[fxN];

                    uint32_t rawOffset = 
                        ((uint32_t)fx.AddressUpperBits << 16) | fx.Address;

                    uint32_t address = 
                        ((uint32_t)instEntry->FixupInformationOffset + 
                            rawOffset) & FIXUP_MASK;

                    int64_t transformsBase = fixupDataBase + address;

                    const int32_t kInstStride = 156;

                    int instCount = 
                        (int)sbsp->InstancedGeometryInstances.size();

                    std::string filePath = 
                        m_Deps.State_MapReader.GetMapFilePath();

                    auto blob = m_Deps.System_MapReader.ReadDataFrom(
                        filePath, transformsBase, kInstStride * instCount);

                    for (int i = 0; i < instCount; ++i)
                    {
                        const int64_t base = (int64_t)i * kInstStride;
                        if (base + kInstStride > (int64_t)blob.size()) break;
                        const uint8_t* p = blob.data() + base;

                        // @0 scale, @4 Matrix3x4 (12 floats), @58 SectionIndex.
                        float scale; memcpy(&scale, p + 0, 4);
                        float m[12];

                        for (int k = 0; k < 12; ++k)
                        {
                            memcpy(&m[k], p + 4 + k * 4, 4);
                        }

                        for (int k = 0; k < 9; ++k)
                        {
                            // rot scale, not translation
                            m[k] *= scale;
                        } 

                        int16_t secIdx16; memcpy(&secIdx16, p + 58, 2);

                        int secIdx = (int)secIdx16;
                        if (secIdx < 0 || secIdx >= (int)lbsp->Meshes.size())
                        {
                            continue;
                        }

                        const Lbsp_MeshesObject& sec = lbsp->Meshes[secIdx];

                        triInst += EmitSection(pageData, *lbspEntry, vbCount,
                            vbInfo, ibInfo, sec, CompFor(secIdx), 
                            geo.TagName.c_str(), m, geo.RenderMesh);
                    }
                }
                else
                {
                    m_Deps.System_Logs.Log("[MeshReader] WARNING: '%s':"
                        " InstancesEntry.ResourceFixups too short"
                        " (Count=%d, need >=10)", geo.TagName.c_str(), 
                        (int)instEntry->ResourceFixups.size());
                }
            }
        }

        m_Deps.System_Logs.Log("[MeshReader] INFO: '%s': %u total tris"
            " (%u clusters + %u instances)", geo.TagName.c_str(), 
            triWorld + triInst, triWorld, triInst);

        ++sbspsProcessed;
    }

    m_Deps.System_Logs.Log("[MeshReader] INFO: processed SBSPs: %d", 
        sbspsProcessed);

    return sbspsProcessed > 0;
}