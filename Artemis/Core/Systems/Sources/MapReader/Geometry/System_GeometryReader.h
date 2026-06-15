// This geometry extraction logic is derived from Reclaimer
//   (https://github.com/Gravemind2401/Reclaimer) by Gravemind2401,
//   licensed under GPL-3.0. The mesh-access theory, buffer layout,
//   unstrip algorithm and instance transform handling were ported
//   from Reclaimer's C# implementation to C++.
// See MeshReader.md for the full Assembly <-> Reclaimer name mapping.

#pragma once

#include <cstdint>
#include <vector>

struct SbspVec3;
struct SbspGeometry;
struct SbspTriangle;
struct Lbsp_MeshesObject;
struct Play_SegmentsEntry;
struct Play_RawPagesEntry;
struct Zone_TagResourcesObject;
struct Sbsp_CompressionInfoEntry_3;
struct Zone_TagResources_ResourceFixupsEntry;

struct BufferInfo
{
    uint32_t DataLength;
    uint32_t Aux; // Vertex Count or Index Format.
};

struct VertexDecodeContext
{
    const uint8_t* Buffer = nullptr;
    uint32_t Count = 0;
    uint32_t Stride = 0;
    bool HasBounds = false;
    float MinX = 0, MinY = 0, MinZ = 0;
    float LengthX = 0, LengthY = 0, LengthZ = 0;
    const float* TransformMatrix = nullptr;
};

using SbspGeometries = std::vector<SbspGeometry>;
using ResourceFixups = Zone_TagResources_ResourceFixupsEntry;
using TagResourcesObject = Zone_TagResourcesObject;
using BufferInfoTable = std::vector<BufferInfo>;
using PageData = std::vector<uint8_t>;
using SegmentsEntry = Play_SegmentsEntry;
using RawPagesEntry = Play_RawPagesEntry;
using CompressionInfoEntry3 = Sbsp_CompressionInfoEntry_3;
using MeshesObject = Lbsp_MeshesObject;
using RenderGeometry = std::vector<SbspTriangle>;

class State_MapReader;
template <typename TObject> class State_MapScnr; struct ScnrObject;
template <typename TObject> class State_MapSbsp; struct SbspObject;
template <typename TObject> class State_MapLbsp; struct LbspObject;
template <typename TObject> class State_MapPlay; struct PlayObject;
template <typename TObject> class State_MapZone; struct ZoneObject;
class System_MapReader;
class System_Logs;

struct Sys_MeshReader_Deps
{
    State_MapReader& State_MapReader;
    State_MapScnr<ScnrObject>& State_MapScnr;
    State_MapSbsp<SbspObject>& State_MapSbsp;
    State_MapLbsp<LbspObject>& State_MapLbsp;
    State_MapPlay<PlayObject>& State_MapPlay;
    State_MapZone<ZoneObject>& State_MapZone;
    System_MapReader& System_MapReader;
    System_Logs& System_Logs;
};

class System_GeometryReader
{
public:
    System_GeometryReader(Sys_MeshReader_Deps deps) : m_Deps(deps) {}
    ~System_GeometryReader() = default;

    bool ReadRenderGeometry(SbspGeometries& out);

private:
    Sys_MeshReader_Deps m_Deps;

    static constexpr uint32_t m_kFixupMask = 0x0FFFFFFF;
    static constexpr uint32_t m_kResourceDatumMask = 0xFFFFu;

    static constexpr int64_t m_kFooterSize = 24;
    static constexpr int64_t m_kFooterVBCountOffset = 0;
    static constexpr int64_t m_kFooterIBCountOffset = 12;

    static constexpr int32_t m_kInfoStride = 28;
    static constexpr int32_t m_kAuxSize = 12;
    static constexpr int32_t m_kInfoAuxOffset = 0;
    static constexpr int32_t m_kInfoDataLengthOffset = 8;

    static constexpr uint32_t m_kVertexBufferStride = 0x24;
    static constexpr int32_t m_kXOffset = 0;
    static constexpr int32_t m_kYOffset = 4;
    static constexpr int32_t m_kZOffset = 8;
    static constexpr uint16_t m_kMeshFlagUnindexed = (1u << 4);

    static constexpr int32_t m_kInstanceStride = 156;
    static constexpr int m_kInstanceFixupFromEnd = 10;
    static constexpr int m_kInstanceMatrixFloats = 12;
    static constexpr int m_kInstanceScaledFloats = 9;
    static constexpr int m_kInstanceScaleOffset = 0;
    static constexpr int m_kInstanceMatrixOffset = 4;
    static constexpr int m_kInstanceSectionOffset = 58;

    PageData ReadResourcePage(uint32_t datum,
        const TagResourcesObject** outEntry,
        const ZoneObject* zone, const PlayObject* play);

    bool ReadFooter(const TagResourcesObject& entry,
        int32_t& outVBCount, int32_t& outIBCount,
        const int64_t fixupDataBase);

    bool ReadBufferInfoTables(const TagResourcesObject& entry,
        int32_t vbCount, int32_t ibCount,
        BufferInfoTable& outVBTable, BufferInfoTable& outIBTable,
        const int64_t fixupDataBase);

    uint32_t EmitSection(const PageData& pageData,
        const TagResourcesObject& entry, int32_t vbCount,
        const BufferInfoTable& vbInfo,
        const BufferInfoTable& ibInfo,
        const MeshesObject& section,
        const CompressionInfoEntry3* compressionInfo,
        const char* tagName, const float* transform,
        RenderGeometry& out);

    uint32_t EmitInstancedGeometry(const PageData& pageData,
        const TagResourcesObject& lbspEntry, int32_t vbCount,
        const BufferInfoTable& vbInfo, const BufferInfoTable& ibInfo,
        const SbspObject* sbsp, const LbspObject* lbsp,
        const ZoneObject* zone, int64_t fixupDataBase,
        SbspGeometry& geometry);

    // --- Helpers ---
    uint32_t ResolveFixupOffset(const ResourceFixups& fixups) const;

    const CompressionInfoEntry3* GetCompressionInfo(int sectionIdx,
        const SbspObject* sbsp);

    void EmitRange(uint32_t start, uint32_t count,
        const VertexDecodeContext& context,
        const uint8_t* ibPointer, bool wide,
        uint32_t idxCount, bool isStrip,
        uint32_t& emitted, RenderGeometry& out);

    void PushTriangle(uint32_t a, uint32_t b, uint32_t c,
        const VertexDecodeContext& context,
        uint32_t& emitted, RenderGeometry& out);

    SbspVec3 ReadVertex(uint32_t idx,
        const VertexDecodeContext& context);

    uint32_t ReadIndex(uint32_t i, bool wide,
        const uint8_t* ibPointer);
};