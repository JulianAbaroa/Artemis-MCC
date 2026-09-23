export module Map.Reader.System:Geometry.ResourcePage;

import :FileLocator;
import :DataStream;
import :Formula;
import Service.Logs.System;
import Map.Reader.State;
import Map.Tag.Type;
import std;

export namespace Map::Reader::System
{
	class GeometryResourcePageService
	{
	private:
		using ZoneObject = Map::Tag::Type::Zone::Object::ZoneObject;
		using TagResourcesObject = Map::Tag::Type::Zone::Object::Zone_TagResourcesObject;
		using PlayObject = Map::Tag::Type::Play::Object::PlayObject;

		using LogsService = Service::Logs::System::LogsService;
		using FileStore = Map::Reader::State::FileStore;
		using FileLocatorService = Map::Reader::System::FileLocatorService;
		using DataStreamService = Map::Reader::System::DataStreamService;
		using FormulaService = Map::Reader::System::FormulaService;

	public:
		GeometryResourcePageService(LogsService& logsService, FileStore& fileStore,
			FileLocatorService& fileLocatorService,
			DataStreamService& dataStreamService,
			FormulaService& m_FormulaService) :
			m_LogsService(logsService), m_FileStore(fileStore),
			m_FileLocatorService(fileLocatorService),
			m_DataStreamService(dataStreamService),
			m_FormulaService(m_FormulaService) {}
		~GeometryResourcePageService() = default;

		auto Read(std::uint32_t datum, const TagResourcesObject** outEntry,
			const ZoneObject* zone, const PlayObject* play) const -> 
			std::vector<std::uint8_t>;

	private:
		LogsService& m_LogsService;
		FileStore& m_FileStore;
		FileLocatorService& m_FileLocatorService;
		DataStreamService& m_DataStreamService;
		FormulaService& m_FormulaService;
	};
}