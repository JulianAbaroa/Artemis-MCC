export module Map.Reader.System:Geometry.Info;

import :DataStream;

import Map.Reader.Type;
import Map.Reader.State;
import Map.Tag.Type;
import std;

export namespace Map::Reader::System
{
    // Reads the footer and the buffer info tables of a geometry resource.
    class GeometryInfoService
    {
    private:
        using TagResourcesObject = Map::Tag::Type::Zone::Object::Zone_TagResourcesObject;
        using BufferInfo = Map::Reader::Type::Geometry::BufferInfo;

        using FileStore = Map::Reader::State::FileStore;
        using DataStreamService = Map::Reader::System::DataStreamService;

    public:
        GeometryInfoService(FileStore& fileStore, DataStreamService& dataStreamService) :
            m_FileStore(fileStore), m_DataStreamService(dataStreamService) {}
        ~GeometryInfoService() = default;

        // Reads how many vertex and index buffers the resource has.
        // param entry: Tag resource that owns the buffers.
        // param outVBCount: Out. Number of vertex buffers.
        // param outIBCount: Out. Number of index buffers.
        // param fixupDataBase: File offset where the fixup data of the zone starts.
        // return: False if the footer cannot be read.
        auto ReadFooter(const TagResourcesObject& entry,
            std::int32_t& outVBCount, std::int32_t& outIBCount,
            std::int64_t fixupDataBase) const -> bool;

        // Reads the size and the auxiliary value of every vertex and index buffer.
        // param outVBTable: Out. One entry per vertex buffer.
        // param outIBTable: Out. One entry per index buffer.
        // return: False if there is nothing to read or the tables are cut short.
        auto ReadBufferInfoTables(const TagResourcesObject& entry,
            std::int32_t vbCount, std::int32_t ibCount,
            std::vector<BufferInfo>& outVBTable,
            std::vector<BufferInfo>& outIBTable,
            std::int64_t fixupDataBase) const -> bool;

    private:
        FileStore& m_FileStore;
        DataStreamService& m_DataStreamService;
    };
}