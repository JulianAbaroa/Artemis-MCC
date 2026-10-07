export module Map.Reader.System:Geometry.MeshDecoder;

import :DataStream;
import :Formula;

import Service.Logs.System;
import Common.Math.Type;
import Map.Reader.Type;
import Map.Reader.State;
import Map.Tag.Type;
import std;

export namespace Map::Reader::System
{
    // Decodes the vertex and index buffers of the mesh sections into triangles.
    class GeometryMeshDecoderService
    {
    private:
        using Vec3 = Common::Math::Type::Vec3;
        using Triangle = Common::Math::Type::Triangle;
        using SbspObject = Map::Tag::Type::Sbsp::Object::SbspObject;
        using LbspObject = Map::Tag::Type::Lbsp::Object::LbspObject;
        using ZoneObject = Map::Tag::Type::Zone::Object::ZoneObject;
        using MeshesObject = Map::Tag::Type::Lbsp::Object::Lbsp_MeshesObject;
        using CompressionInfoEntry3 = Map::Tag::Type::Sbsp::Structure::Sbsp_CompressionInfoEntry_3;
        using TagResourcesObject = Map::Tag::Type::Zone::Object::Zone_TagResourcesObject;
        using VertexDecodeContext = Map::Reader::Type::Geometry::VertexDecodeContext;
        using BufferInfo = Map::Reader::Type::Geometry::BufferInfo;

        using LogsService = Service::Logs::System::LogsService;
        using FileStore = Map::Reader::State::FileStore;
        using DataStreamService = Map::Reader::System::DataStreamService;
        using FormulaService = Map::Reader::System::FormulaService;

    public:
        GeometryMeshDecoderService(LogsService& logsService, FileStore& fileStore,
            DataStreamService& dataStreamService, FormulaService& formulaService) :
            m_LogsService(logsService), m_FileStore(fileStore),
            m_DataStreamService(dataStreamService), m_FormulaService(formulaService) {}
        ~GeometryMeshDecoderService() = default;

        // Decodes one mesh section into triangles.
        // param pageData: Raw page that holds the buffers.
        // param entry: Tag resource that owns the buffers and their fixups.
        // param vbCount: Number of vertex buffers of the resource.
        // param vbInfo: Info of each vertex buffer.
        // param ibInfo: Info of each index buffer.
        // param compressionInfo: Bounds to restore the compressed positions. Null if the positions are not compressed.
        // param tagName: Name of the tag, for the logs. Can be null.
        // param transformMatrix: Moves the vertices to world space. Null if they are already there.
        // param out: Out. The triangles are appended to it.
        // return: Number of triangles added.
        auto EmitSection(const std::vector<std::uint8_t>& pageData,
            const TagResourcesObject& entry, std::int32_t vbCount,
            const std::vector<BufferInfo>& vbInfo,
            const std::vector<BufferInfo>& ibInfo,
            const MeshesObject& section,
            const CompressionInfoEntry3* compressionInfo,
            const char* tagName, const float* transformMatrix,
            std::vector<Triangle>& out) const -> std::uint32_t;

        // Decodes every instance of the instanced geometry of a BSP, each one placed with its own transform.
        // param lbspEntry: Tag resource of the lightmap BSP, which owns the buffers.
        // param fixupDataBase: File offset where the fixup data of the zone starts.
        // return: Number of triangles added.
        auto EmitInstancedGeometry(const std::vector<std::uint8_t>& pageData,
            const TagResourcesObject& lbspEntry, std::int32_t vbCount,
            const std::vector<BufferInfo>& vbInfo,
            const std::vector<BufferInfo>& ibInfo,
            const SbspObject* sbsp, const LbspObject* lbsp,
            const ZoneObject* zone, std::int64_t fixupDataBase,
            const char* tagName, std::vector<Triangle>& out) const -> std::uint32_t;

        // return: Null if the section has no compression info.
        auto GetCompressionInfo(int sectionIdx,
            const SbspObject* sbsp) const -> const CompressionInfoEntry3*;

    private:
        LogsService& m_LogsService;
        FileStore& m_FileStore;
        DataStreamService& m_DataStreamService;
        FormulaService& m_FormulaService;

        // Emits the triangles of a range of indices, as a list or as a strip.
        // param start: First index of the range.
        // param count: Number of indices in the range.
        // param wide: True if the indices are 32 bit, false if they are 16 bit.
        // param idxCount: Number of indices in the buffer. The range is cut to it.
        // param emitted: In/out. Counts the triangles added.
        auto EmitRange(std::uint32_t start, std::uint32_t count,
            const VertexDecodeContext& context,
            const std::uint8_t* ibPointer, bool wide,
            std::uint32_t idxCount, bool isStrip,
            std::uint32_t& emitted, std::vector<Triangle>& out) const -> void;

        // Adds a triangle. Skips it if it is degenerate or an index is out of range.
        auto PushTriangle(std::uint32_t a, std::uint32_t b, std::uint32_t c,
            const VertexDecodeContext& context,
            std::uint32_t& emitted, std::vector<Triangle>& out) const -> void;

        // Reads a vertex position, restoring the bounds and applying the transform if the context has them.
        auto ReadVertex(std::uint32_t idx,
            const VertexDecodeContext& context) const -> Vec3;

        auto ReadIndex(std::uint32_t i, bool wide,
            const std::uint8_t* ibPointer) const -> std::uint32_t;
    };
}