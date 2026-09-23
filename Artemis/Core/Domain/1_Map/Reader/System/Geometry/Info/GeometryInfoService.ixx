export module Map.Reader.System:Geometry.Info;

import :DataStream;
import Map.Reader.Type;
import Map.Reader.State;
import Map.Tag.Type;
import std;

export namespace Map::Reader::System
{
	class GeometryInfoService
	{
	private:
		using TagResourcesObject = Map::Tag::Type::Zone::Object::Zone_TagResourcesObject;
		using BufferInfo = Map::Reader::Type::Geometry::BufferInfo;

		using FileStore = Map::Reader::State::FileStore;
		using DataStreamService = Map::Reader::System::DataStreamService;

		static constexpr std::int64_t m_kFooterSize{ 24 };
		static constexpr std::int64_t m_kFooterVBCountOffset{ 0 };
		static constexpr std::int64_t m_kFooterIBCountOffset{ 12 };

		static constexpr std::int32_t m_kInfoStride{ 28 };
		static constexpr std::int32_t m_kAuxSize{ 12 };
		static constexpr std::int32_t m_kInfoAuxOffset{ 0 };
		static constexpr std::int32_t m_kInfoDataLengthOffset{ 8 };

	public:
		GeometryInfoService(FileStore& fileStore, DataStreamService& dataStreamService) :
			m_FileStore(fileStore), m_DataStreamService(dataStreamService) {}
		~GeometryInfoService() = default;

		auto ReadFooter(const TagResourcesObject& entry,
			std::int32_t& outVBCount, std::int32_t& outIBCount,
			std::int64_t fixupDataBase) const -> bool;

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