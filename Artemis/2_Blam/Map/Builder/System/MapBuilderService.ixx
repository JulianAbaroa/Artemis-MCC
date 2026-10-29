export module Blam.Map.Builder.System;

import Common.Log.System;
import Blam.Map.Reader.State;
import Blam.Map.Reader.System;
import Blam.Map.Tag.State;
import Blam.Map.Tag.System;
import std;

export namespace Blam::Map::Builder::System
{
    // Reads every tag of the loaded map that has a descriptor and stores it in the tag catalog.
    class MapBuilderService
    {
    private:
        using TagCatalog = Blam::Map::Tag::State::TagCatalog;

        using Log = Common::Log::System::Log;
        using FileStore = Blam::Map::Reader::State::FileStore;
        using TagIndexStore = Blam::Map::Reader::State::TagIndexStore;
        using FileLocatorService = Blam::Map::Reader::System::FileLocatorService;
        using TagReferenceService = Blam::Map::Reader::System::TagReferenceService;
        using TagGroupService = Blam::Map::Reader::System::TagGroupService;

    public:
        MapBuilderService(Log& log, TagCatalog& tagCatalog,
            FileStore& fileStore, TagIndexStore& tagIndexStore,
            FileLocatorService& fileLocatorService,
            TagReferenceService& tagReferenceService) :
            m_Log(log), m_TagCatalog(tagCatalog), m_FileStore(fileStore),
            m_TagIndexStore(tagIndexStore), m_FileLocatorService(fileLocatorService),
            m_TagReferenceService(tagReferenceService) {}
        ~MapBuilderService() = default;

        // Fills the tag catalog from the loaded map and freezes it. Logs the result.
        // note: Needs the map to be loaded first.
        auto LoadForMap() -> void;

        // Empties the tag catalog.
        auto Cleanup() -> void;

    private:
        Log& m_Log;
        TagCatalog& m_TagCatalog;
        FileStore& m_FileStore;
        TagIndexStore& m_TagIndexStore;
        FileLocatorService& m_FileLocatorService;
        TagReferenceService& m_TagReferenceService;

        auto LoadTags(std::ifstream& file, TagGroupService& reader) -> void;
    };
}