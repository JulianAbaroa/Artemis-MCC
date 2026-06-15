// This geometry extraction logic is derived from Reclaimer
//   (https://github.com/Gravemind2401/Reclaimer) by Gravemind2401,
//   licensed under GPL-3.0. The mesh-access theory, buffer layout,
//   unstrip algorithm and instance transform handling were ported
//   from Reclaimer's C# implementation to C++.
// See MeshReader.md for the full Assembly <-> Reclaimer name mapping.

#include "pch.h"

#include "System_GeometryReader.h"

#include "Core/Types/Sources/MapReader/MapMagics.h"
#include "Core/Types/Sources/Static/World/SbspGeometry.h"

#include "Core/States/Sources/MapReader/State_MapReader.h"
#include "Core/States/Sources/MapReader/Tags/State_MapScnr.h"
#include "Core/States/Sources/MapReader/Tags/State_MapSbsp.h"
#include "Core/States/Sources/MapReader/Tags/State_MapLbsp.h"
#include "Core/States/Sources/MapReader/Tags/State_MapPlay.h"
#include "Core/States/Sources/MapReader/Tags/State_MapZone.h"

#include "../System_MapReader.h"
#include "Core/Systems/Other/Logs/System_Logs.h"

bool System_GeometryReader::ReadRenderGeometry(
    SbspGeometries& out)
{
    const ZoneObject* zone = m_Deps.State_MapZone.GetFirstZone();
    const PlayObject* play = m_Deps.State_MapPlay.GetFirstPlay();
    if (!zone || !play) return false;

    auto& sys_reader = m_Deps.System_MapReader;

    const int64_t fixupDataBase = sys_reader.ToFileOffset(
        sys_reader.Expand(zone->Data.FixupInformation.Pointer));

    int32_t processed = 0;

    for (SbspGeometry& geometry : out)
    {
        const char* tagName = geometry.TagName.c_str();

        const SbspObject* sbsp = 
            m_Deps.State_MapSbsp.Get(geometry.TagName);
        if (!sbsp) continue;

        const LbspObject* lbsp = 
            m_Deps.State_MapLbsp.Get(geometry.TagName);
        if (!lbsp || lbsp->Meshes.empty()) continue;

        const TagResourcesObject* lbspEntry = nullptr;

        PageData pageData = this->ReadResourcePage(
            lbsp->Data.ZoneAssetDatum, &lbspEntry, zone, play);

        if (pageData.empty() || !lbspEntry)
        {
            m_Deps.System_Logs.Log("[GeometryReader] INFO: '%s':"
                " LBSP page empty/omitted", tagName);
            continue;
        }

        int32_t vbCount = 0, ibCount = 0;
        if (!this->ReadFooter(*lbspEntry, vbCount, ibCount, 
            fixupDataBase) || vbCount <= 0)
        {
            m_Deps.System_Logs.Log("[GeometryReader] WARNING:"
                " '%s': invalid footer", tagName);
            continue;
        }

        BufferInfoTable vbInfo, ibInfo;
        if (!this->ReadBufferInfoTables(*lbspEntry, vbCount, 
            ibCount, vbInfo, ibInfo, fixupDataBase))
        {
            m_Deps.System_Logs.Log("[GeometryReader] WARNING:"
                " '%s': invalid info-arrays", tagName);
            continue;
        }

        uint32_t trianglesWorld = 0;
        for (const auto& cluster : sbsp->Clusters)
        {
            int sectionIdx = cluster.MeshIndex;
            if (sectionIdx < 0 || sectionIdx >=
                (int)lbsp->Meshes.size())
            {
                continue;
            }

            const MeshesObject& section = 
                lbsp->Meshes[sectionIdx];

            trianglesWorld += this->EmitSection(pageData, 
                *lbspEntry, vbCount, vbInfo, ibInfo, 
                section, nullptr, tagName, nullptr, 
                geometry.RenderGeometry);
        }

        uint32_t trianglesInstances = this->EmitInstancedGeometry(
            pageData, *lbspEntry, vbCount, vbInfo, ibInfo,
            sbsp, lbsp, zone, fixupDataBase, geometry);

        m_Deps.System_Logs.Log("[GeometryReader] INFO:"
            " '%s': %u total tris (%u clusters + %u instances)", 
            tagName, trianglesWorld + trianglesInstances, 
            trianglesWorld, trianglesInstances);

        ++processed;
    }

    m_Deps.System_Logs.Log("[GeometryReader] INFO:"
        " processed SBSPs: %d", processed);

    return processed > 0;
}

PageData System_GeometryReader::ReadResourcePage(
    uint32_t datum, const TagResourcesObject** outEntry,
    const ZoneObject* zone, const PlayObject* play)
{
    auto& sys_reader = m_Deps.System_MapReader;
    auto& sta_reader = m_Deps.State_MapReader;

    int resourceIdx = (int)(datum & 0xFFFF);
    if (resourceIdx < 0 || resourceIdx >=
        (int)zone->TagResources.size())
    {
        return {};
    }

    const TagResourcesObject& entry =
        zone->TagResources[resourceIdx];

    if (outEntry) *outEntry = &entry;

    int segmentIdx = (int)entry.PlaySegmentIndex;
    if (segmentIdx < 0 || segmentIdx >=
        (int)play->Segments.size())
    {
        return {};
    }

    const SegmentsEntry& segment = play->Segments[segmentIdx];

    int pageIdx = (int)segment.PrimaryPageIndex;
    int32_t segmentOffset = segment.PrimarySegmentOffset;

    if (segment.SecondaryPageIndex >= 0)
    {
        pageIdx = (int)segment.SecondaryPageIndex;
        segmentOffset = segment.SecondarySegmentOffset;
    }

    if (pageIdx < 0 || pageIdx >= (int)play->RawPages.size())
    {
        return {};
    }

    const RawPagesEntry* page = &play->RawPages[pageIdx];

    if (page->BlockOffset == 0xFFFFFFFFu ||
        page->CompressedBlockSize == 0)
    {
        pageIdx = (int)segment.PrimaryPageIndex;
        segmentOffset = segment.PrimarySegmentOffset;

        if (pageIdx < 0 || pageIdx >= (int)play->RawPages.size())
        {
            return {};
        }

        page = &play->RawPages[pageIdx];
    }

    if (page->SharedCacheIndex >= 0)
    {
        std::string externalPath;

        int externalIdx = (int)page->SharedCacheIndex;
        if (externalIdx <
            (int)play->ExternalCacheReferences.size())
        {
            const char* mapPath =
                reinterpret_cast<const char*>(
                    &play->ExternalCacheReferences[
                        externalIdx].MapPath);

            externalPath =
                sys_reader.ResolveExternalCachePath(mapPath);
        }

        if (externalPath.empty())
        {
            m_Deps.System_Logs.Log("[GeometryReader] WARNING:"
                " Resource External cache resource without"
                " a resolvable path. (idx=%d)", externalIdx);
            return {};
        }

        int64_t externalFilePosition =
            sys_reader.ToResourceOffset(
                (int64_t)page->BlockOffset);

        int32_t externalSegmentLength =
            (int32_t)page->UncompressedBlockSize - segmentOffset;
        if (externalSegmentLength <= 0) return {};

        return sys_reader.ReadSegmentFrom(
            externalPath, externalFilePosition,
            (int32_t)page->CompressedBlockSize,
            (int32_t)page->UncompressedBlockSize,
            segmentOffset, externalSegmentLength);
    }

    int64_t filePosition = sys_reader.
        ToResourceOffset((int64_t)page->BlockOffset);

    int32_t segmentLength =
        (int32_t)page->UncompressedBlockSize - segmentOffset;
    if (segmentLength <= 0) return {};

    return sys_reader.ReadSegment(filePosition,
        (int32_t)page->CompressedBlockSize,
        (int32_t)page->UncompressedBlockSize,
        segmentOffset, segmentLength);
}


bool System_GeometryReader::ReadFooter(
    const TagResourcesObject& entry, int32_t& outVBCount, 
    int32_t& outIBCount, const int64_t fixupDataBase)
{
    auto& sys_reader = m_Deps.System_MapReader;
    auto& sta_reader = m_Deps.State_MapReader;

    std::string filePath = sta_reader.GetMapFilePath();

    int64_t footerPosition = fixupDataBase +
        entry.FixupInformationOffset +
        entry.FixupInformationLength - m_kFooterSize;

    auto buffer = sys_reader.ReadDataFrom(
        filePath, footerPosition, m_kFooterSize);

    if (buffer.size() < m_kFooterSize) return false;

    memcpy(&outVBCount, buffer.data() + 
        m_kFooterVBCountOffset, sizeof(outVBCount));

    memcpy(&outIBCount, buffer.data() + 
        m_kFooterIBCountOffset, sizeof(outIBCount));

    return true;
}

bool System_GeometryReader::ReadBufferInfoTables(
    const TagResourcesObject& entry, int32_t vbCount,
    int32_t ibCount, BufferInfoTable& outVBTable,
    BufferInfoTable& outIBTable, const int64_t fixupDataBase)
{
    auto& sys_reader = m_Deps.System_MapReader;
    auto& sta_reader = m_Deps.State_MapReader;

    int64_t blockBase = fixupDataBase +
        entry.FixupInformationOffset;

    int64_t vbInfoBytes = (int64_t)vbCount * m_kInfoStride;
    int64_t auxBytes = (int64_t)vbCount * m_kAuxSize;
    int64_t ibInfoBytes = (int64_t)ibCount * m_kInfoStride;

    int64_t totalBytes = vbInfoBytes + auxBytes + ibInfoBytes;
    if (totalBytes <= 0) return false;

    std::string filePath = sta_reader.GetMapFilePath();

    auto infoBuffer = sys_reader.ReadDataFrom(
        filePath, blockBase, (int32_t)totalBytes);

    if ((int64_t)infoBuffer.size() < totalBytes) return false;

    outVBTable.resize(vbCount);
    for (int i = 0; i < vbCount; ++i)
    {
        const uint8_t* pointer = infoBuffer.data() +
            (int64_t)i * m_kInfoStride;

        uint32_t vertexCount = 0;
        uint32_t dataLength = 0;

        memcpy(&vertexCount, pointer + 
            m_kInfoAuxOffset, sizeof(vertexCount));

        memcpy(&dataLength, pointer + 
            m_kInfoDataLengthOffset, sizeof(dataLength));

        outVBTable[i] = { dataLength, vertexCount };
    }

    int64_t ibBase = vbInfoBytes + auxBytes;

    outIBTable.resize(ibCount);
    for (int i = 0; i < ibCount; ++i)
    {
        const uint8_t* pointer = infoBuffer.data() +
            ibBase + (int64_t)i * m_kInfoStride;

        uint32_t indexFormat = 0;
        uint32_t dataLength = 0;

        memcpy(&indexFormat, pointer + 
            m_kInfoAuxOffset, sizeof(indexFormat));

        memcpy(&dataLength, pointer + 
            m_kInfoDataLengthOffset, sizeof(dataLength));

        outIBTable[i] = { dataLength, indexFormat };
    }

    return true;
}

uint32_t System_GeometryReader::EmitSection(
    const PageData& pageData, const TagResourcesObject& entry,
    int32_t vbCount, const BufferInfoTable& vbInfo,
    const BufferInfoTable& ibInfo, const MeshesObject& sec,
    const CompressionInfoEntry3* compressionInfo,
    const char* tagName, const float* transformMatrix,
    RenderGeometry& out)
{
    int vbIdx = sec.VertexBufferIndex1;
    int ibIdx = sec.IndexBufferIndex;

    if (vbIdx < 0 || vbIdx >= vbCount) return 0;
    if (vbIdx >= (int)vbInfo.size()) return 0;
    if (vbIdx >= (int)entry.ResourceFixups.size()) return 0;

    uint32_t vbOffset = this->ResolveFixupOffset(
        entry.ResourceFixups[vbIdx]);

    uint32_t vbLength = vbInfo[vbIdx].DataLength;

    if ((int64_t)vbOffset + vbLength > (int64_t)pageData.size())
    {
        return 0;
    }

    const uint8_t* vertexBuffer = pageData.data() + vbOffset;

    uint32_t vCountInfo = vbInfo[vbIdx].Aux;
    uint32_t stride = m_kVertexBufferStride;

    if (vCountInfo > 0 && (vbLength % vCountInfo) == 0)
    {
        stride = vbLength / vCountInfo;
    }
    else if (vCountInfo > 0)
    {
        m_Deps.System_Logs.Log("[GeometryReader] WARNING:"
            " '%s': VB %d DataLength=%u not divisible by"
            " VertexCount=%u, stride fallback 0x%X "
            " (VertexType=%u).", tagName ? tagName : "?",
            vbIdx, vbLength, vCountInfo, m_kVertexBufferStride,
            (unsigned)sec.VertexType);
    }

    if (stride == 0) return 0;
    uint32_t vertexCount = vbLength / stride;
    if (vertexCount == 0) return 0;

    bool hasBounds = false;

    float minX = 0, minY = 0, minZ = 0, 
        lengthX = 1, lengthY = 1, lengthZ = 1;

    if (compressionInfo != nullptr)
    {
        const auto& bx = compressionInfo->PositionBoundsX;
        const auto& by = compressionInfo->PositionBoundsY;
        const auto& bz = compressionInfo->PositionBoundsZ;

        hasBounds = !(bx.Min == bx.Max && 
            by.Min == by.Max && bz.Min == bz.Max);

        if (hasBounds)
        {
            minX = bx.Min; lengthX = bx.Max - bx.Min;
            minY = by.Min; lengthY = by.Max - by.Min;
            minZ = bz.Min; lengthZ = bz.Max - bz.Min;
        }
    }

    VertexDecodeContext context{};
    context.Buffer = vertexBuffer;
    context.Count = vertexCount;
    context.Stride = stride;
    context.HasBounds = hasBounds;
    context.MinX = minX;
    context.MinY = minY;
    context.MinZ = minZ;
    context.LengthX = lengthX;
    context.LengthY = lengthY;
    context.LengthZ = lengthZ;
    context.TransformMatrix = transformMatrix;

    uint32_t emitted = 0;

    bool unindexed = (ibIdx < 0) || 
        (sec.MeshFlags & m_kMeshFlagUnindexed);

    if (unindexed)
    {
        for (uint32_t i = 0; i + 2 < vertexCount; ++i)
        {
            if (i & 1)
            {
                this->PushTriangle(i, i + 2, i + 1, 
                    context, emitted, out);
            }
            else
            {
                this->PushTriangle(i, i + 1, i + 2, 
                    context, emitted, out);
            }
        }

        return emitted;
    }

    if (ibIdx >= (int)ibInfo.size()) return emitted;

    int fixupIB = vbCount * 2 + ibIdx;
    if (fixupIB < 0 || fixupIB >=
        (int)entry.ResourceFixups.size())
    {
        return emitted;
    }

    uint32_t ibOffset = this->ResolveFixupOffset(
        entry.ResourceFixups[fixupIB]);

    uint32_t ibLength = ibInfo[ibIdx].DataLength;

    if ((int64_t)ibOffset + ibLength >
        (int64_t)pageData.size())
    {
        return emitted;
    }

    bool wide = (vertexCount > 
        (std::numeric_limits<uint16_t>::max)());

    uint32_t idxStride = wide ? 
        sizeof(uint32_t) : sizeof(uint16_t);

    uint32_t idxCount = ibLength / idxStride;
    const uint8_t* ibPointer = pageData.data() + ibOffset;

    bool isStrip = (sec.IndexBufferType == 5) || 
        (sec.IndexBufferType == 0);

    if (!sec.Parts.empty())
    {
        for (const auto& part : sec.Parts)
        {
            this->EmitRange(part.IndexStart, part.IndexCount,
                context, ibPointer, wide, idxCount, 
                isStrip, emitted, out);
        }
    }
    else
    {
        this->EmitRange(0, idxCount,
            context, ibPointer, wide, idxCount, 
            isStrip, emitted, out);
    }

    return emitted;
}

uint32_t System_GeometryReader::EmitInstancedGeometry(
    const PageData& pageData, const TagResourcesObject& lbspEntry,
    int32_t vbCount, const BufferInfoTable& vbInfo,
    const BufferInfoTable& ibInfo, const SbspObject* sbsp,
    const LbspObject* lbsp, const ZoneObject* zone,
    int64_t fixupDataBase, SbspGeometry& geometry)
{
    if (sbsp->InstancedGeometryInstances.empty()) return 0;

    int instResIdx = (int)(sbsp->Data.ZoneAssetDatum5 & m_kResourceDatumMask);
    if (instResIdx < 0 || instResIdx >= (int)zone->TagResources.size()) return 0;

    const TagResourcesObject& instEntry = zone->TagResources[instResIdx];

    int fixupIdx = (int)instEntry.ResourceFixups.size() - m_kInstanceFixupFromEnd;
    if (fixupIdx < 0 || fixupIdx >= (int)instEntry.ResourceFixups.size())
    {
        m_Deps.System_Logs.Log("[GeometryReader] WARNING: '%s':"
            " InstancesEntry.ResourceFixups too short"
            " (Count=%d, need >=%d)", geometry.TagName.c_str(),
            (int)instEntry.ResourceFixups.size(), m_kInstanceFixupFromEnd);
        return 0;
    }

    const auto& fixup = instEntry.ResourceFixups[fixupIdx];

    uint32_t rawOffset = ((uint32_t)fixup.AddressUpperBits << 16) | fixup.Address;
    uint32_t address = ((uint32_t)instEntry.FixupInformationOffset + rawOffset) & m_kFixupMask;
    int64_t transformsBase = fixupDataBase + address;

    int instCount = (int)sbsp->InstancedGeometryInstances.size();
    std::string filePath = m_Deps.State_MapReader.GetMapFilePath();

    auto blob = m_Deps.System_MapReader.ReadDataFrom(
        filePath, transformsBase, m_kInstanceStride * instCount);

    uint32_t emitted = 0;

    for (int i = 0; i < instCount; ++i)
    {
        const int64_t base = (int64_t)i * m_kInstanceStride;
        if (base + m_kInstanceStride > (int64_t)blob.size()) break;

        const uint8_t* pointer = blob.data() + base;

        float scale;
        memcpy(&scale, pointer + m_kInstanceScaleOffset, sizeof(scale));

        float m[m_kInstanceMatrixFloats]{};
        for (int k = 0; k < m_kInstanceMatrixFloats; ++k)
        {
            memcpy(&m[k], pointer + m_kInstanceMatrixOffset + k * (int)sizeof(float), sizeof(float));
        }

        for (int k = 0; k < m_kInstanceScaledFloats; ++k)
        {
            m[k] *= scale;
        }

        int16_t sectionIdx16;
        memcpy(&sectionIdx16, pointer + m_kInstanceSectionOffset, sizeof(sectionIdx16));

        int sectionIdx = (int)sectionIdx16;
        if (sectionIdx < 0 || sectionIdx >= (int)lbsp->Meshes.size()) continue;

        const MeshesObject& section = lbsp->Meshes[sectionIdx];

        emitted += this->EmitSection(pageData, lbspEntry, vbCount,
            vbInfo, ibInfo, section, this->GetCompressionInfo(sectionIdx, sbsp),
            geometry.TagName.c_str(), m, geometry.RenderGeometry);
    }

    return emitted;
}

// --- Helpers ---

uint32_t System_GeometryReader::ResolveFixupOffset(
    const ResourceFixups& fixups) const
{
    uint32_t offset = (uint32_t)fixups.Address | 
        (uint32_t)fixups.AddressUpperBits << 16 |
        (uint32_t)fixups.AddressLocationHighBits << 24;

    return offset & m_kFixupMask;
}

const CompressionInfoEntry3* System_GeometryReader::GetCompressionInfo(
    int sectionIdx, const SbspObject* sbsp)
{
    if (sectionIdx < 0 || sectionIdx >=
        (int)sbsp->CompressionInfo_3.size())
    {
        return nullptr;
    }

    return &sbsp->CompressionInfo_3[sectionIdx];
}

void System_GeometryReader::EmitRange(
    uint32_t start, uint32_t count,
    const VertexDecodeContext& context,
    const uint8_t* ibPointer, bool wide,
    uint32_t idxCount, bool isStrip,
    uint32_t& emitted, RenderGeometry& out)
{
    if (count == 0) return;

    uint64_t end = (uint64_t)start + count;
    if (end > idxCount) end = idxCount;
    if (start >= end) return;

    if (!isStrip)
    {
        for (uint64_t i = start; i + 2 < end; i += 3)
        {
            this->PushTriangle(
                this->ReadIndex((uint32_t)i, wide, ibPointer),
                this->ReadIndex((uint32_t)i + 1, wide, ibPointer),
                this->ReadIndex((uint32_t)i + 2, wide, ibPointer),
                context, emitted, out);
        }

        return;
    }

    uint32_t i0 = 0, i1 = 0, i2 = 0;
    uint64_t pos = 0;

    for (uint64_t i = start; i < end; ++i)
    {
        uint32_t idx = this->ReadIndex(
            (uint32_t)i, wide, ibPointer);

        i0 = i1;
        i1 = i2;
        i2 = idx;

        if (pos++ < 2) continue;
        if (i0 == i1 || i0 == i2 || i1 == i2) continue;

        if (pos & 1)
        {
            this->PushTriangle(i0, i1, i2,
                context, emitted, out);
        }
        else
        {
            this->PushTriangle(i0, i2, i1,
                context, emitted, out);
        }
    }
}

void System_GeometryReader::PushTriangle(
    uint32_t a, uint32_t b, uint32_t c,
    const VertexDecodeContext& context, uint32_t& emitted,
    RenderGeometry& out)
{
    if (a == b || b == c || a == c) return;
    if (a >= context.Count || b >= context.Count ||
        c >= context.Count) return;

    SbspTriangle triangle{};
    triangle.V0 = this->ReadVertex(a, context);
    triangle.V1 = this->ReadVertex(b, context);
    triangle.V2 = this->ReadVertex(c, context);

    out.push_back(triangle);
    ++emitted;
}

SbspVec3 System_GeometryReader::ReadVertex(
    uint32_t idx, const VertexDecodeContext& context)
{
    if (idx >= context.Count) return SbspVec3{};

    const uint8_t* pointer = context.Buffer +
        (size_t)idx * context.Stride;

    float x{}, y{}, z{};

    memcpy(&x, pointer + m_kXOffset, sizeof(x));
    memcpy(&y, pointer + m_kYOffset, sizeof(y));
    memcpy(&z, pointer + m_kZOffset, sizeof(z));

    if (context.HasBounds)
    {
        x = context.MinX + x * context.LengthX;
        y = context.MinY + y * context.LengthY;
        z = context.MinZ + z * context.LengthZ;
    }

    if (context.TransformMatrix)
    {
        const float* m = context.TransformMatrix;
        float worldX = x * m[0] + y * m[3] + z * m[6] + m[9];
        float worldY = x * m[1] + y * m[4] + z * m[7] + m[10];
        float worldZ = x * m[2] + y * m[5] + z * m[8] + m[11];

        return { worldX, worldY, worldZ };
    }

    return { x, y, z };
}

uint32_t System_GeometryReader::ReadIndex(
    uint32_t i, bool wide, const uint8_t* ibPointer)
{
    if (wide)
    {
        uint32_t value;
        memcpy(&value, ibPointer + (size_t)i * sizeof(value), sizeof(value));
        return value;
    }

    uint16_t value;
    memcpy(&value, ibPointer + (size_t)i * sizeof(value), sizeof(value));
    return value;
}