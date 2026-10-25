export module Map.Reader.System:MapLoader;

import :FileLocator;
import :FileNames;
import :Header;
import :TagIndex;

import Common.Log.System;
import Map.Reader.Type;
import Map.Reader.State;
import std;

export namespace Map::Reader::System
{
    // Loads the '.map' file: its header, tag index and tag names.
    // note: Based on Assembly (https://github.com/xboxchaos/assembly) and Reclaimer (https://github.com/Gravemind2401/Reclaimer).
    class MapLoaderService
    {
    private:
        using HeaderInfo = Map::Reader::Type::Info::HeaderInfo;

        using Log = Common::Log::System::Log;
        using FileStore = Map::Reader::State::FileStore;
        using TagIndexStore = Map::Reader::State::TagIndexStore;
        using FileLocatorService = Map::Reader::System::FileLocatorService;
        using FileNamesService = Map::Reader::System::FileNamesService;
        using HeaderService = Map::Reader::System::HeaderService;
        using TagIndexService = Map::Reader::System::TagIndexService;

    public:
        explicit MapLoaderService(Log& log, FileStore& fileStore,
            TagIndexStore& tagIndexStore, FileLocatorService& fileLocatorService,
            FileNamesService& fileNamesService, HeaderService& headerService,
            TagIndexService& tagIndexService) : m_Log(log),
            m_FileStore(fileStore), m_TagIndexStore(tagIndexStore),
            m_FileLocatorService(fileLocatorService),
            m_FileNamesService(fileNamesService), m_HeaderService(headerService),
            m_TagIndexService(tagIndexService) {}
        ~MapLoaderService() = default;

        // Takes a cache file the engine opened: campaign and shared only record their path, any other file is loaded as
        // the map (LoadMap).
        // note: Decides by the file name only. Runs on the thread that reports the file (today the game thread, in the hook).
        auto OpenFile(const std::string& filePath) -> void;

        // Reads the map and marks it as loaded in the file store, or as not loaded if any step fails. The result is logged.
        auto LoadMap(const std::string& filePath) -> void;

        // Clears the header, the file store and the tag index store.
        auto Cleanup() -> void;

    private:
        Log& m_Log;
        FileStore& m_FileStore;
        TagIndexStore& m_TagIndexStore;
        FileLocatorService& m_FileLocatorService;
        FileNamesService& m_FileNamesService;
        HeaderService& m_HeaderService;
        TagIndexService& m_TagIndexService;

        HeaderInfo m_HeaderInfo{};
    };
}