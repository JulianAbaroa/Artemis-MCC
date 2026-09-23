export module Map.Reader.System:MapLoader;

import :FileLocator;
import :FileNames;
import :Header;
import :TagIndex;
import Service.Logs.System;
import Map.Reader.Type;
import Map.Reader.State;
import std;

export namespace Map::Reader::System
{
	class MapLoaderService
	{
	private:
		using HeaderInfo = Map::Reader::Type::Info::HeaderInfo;

		using LogsService = Service::Logs::System::LogsService;
		using FileStore = Map::Reader::State::FileStore;
		using TagIndexStore = Map::Reader::State::TagIndexStore;
		using FileLocatorService = Map::Reader::System::FileLocatorService;
		using FileNamesService = Map::Reader::System::FileNamesService;
		using HeaderService = Map::Reader::System::HeaderService;
		using TagIndexService = Map::Reader::System::TagIndexService;

	public:
		explicit MapLoaderService(LogsService& logsService, FileStore& fileStore,
			TagIndexStore& m_TagIndexStore, FileLocatorService& fileLocatorService,
			FileNamesService& fileNamesService, HeaderService& headerService,
			TagIndexService& tagIndexService) : m_LogsService(logsService),
			m_FileStore(fileStore), m_TagIndexStore(m_TagIndexStore), 
			m_FileLocatorService(fileLocatorService), 
			m_FileNamesService(fileNamesService), m_HeaderService(headerService),
			m_TagIndexService(tagIndexService) {}
		~MapLoaderService() = default;

		auto LoadMap(const std::string& filePath) -> void;
		auto Cleanup() -> void;

	private:
		LogsService& m_LogsService;
		FileStore& m_FileStore;
		TagIndexStore& m_TagIndexStore;
		FileLocatorService& m_FileLocatorService;
		FileNamesService& m_FileNamesService;
		HeaderService& m_HeaderService;
		TagIndexService& m_TagIndexService;

		HeaderInfo m_HeaderInfo;
	};
}