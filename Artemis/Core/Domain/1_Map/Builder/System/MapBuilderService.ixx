export module Map.Builder.System;

import Service.Logs.System;
import Map.Reader.State;
import Map.Reader.System;
import Map.Tag.State;
import Map.Tag.System;
import std;

export namespace Map::Builder::System
{
    class MapBuilderService
    {
    private:
        using TagCatalog = Map::Tag::State::TagCatalog;

        using LogsService = Service::Logs::System::LogsService;
        using FileStore = Map::Reader::State::FileStore;
        using TagIndexStore = Map::Reader::State::TagIndexStore;
        using FileLocatorService = Map::Reader::System::FileLocatorService;
        using TagResolverService = Map::Reader::System::TagResolverService;
        using TagGroupService = Map::Reader::System::TagGroupService;

    public:
        MapBuilderService(LogsService& logsService, TagCatalog& tagCatalog,
            FileStore& fileStore, TagIndexStore& m_TagIndexStore,
            FileLocatorService& fileLocatorService, 
            TagResolverService& tagResolverService) :
            m_LogsService(logsService), m_TagCatalog(tagCatalog), m_FileStore(fileStore),
            m_TagIndexStore(m_TagIndexStore), m_FileLocatorService(fileLocatorService),
            m_TagResolverService(tagResolverService) {}
        ~MapBuilderService() = default;

        auto LoadForMap() -> void;

        auto Cleanup() -> void;

    private:
        LogsService& m_LogsService;
        TagCatalog& m_TagCatalog;
        FileStore& m_FileStore;
        TagIndexStore& m_TagIndexStore;
        FileLocatorService& m_FileLocatorService;
        TagResolverService& m_TagResolverService;

        auto LoadTags(std::ifstream& file, TagGroupService& reader) -> void;
    };
}