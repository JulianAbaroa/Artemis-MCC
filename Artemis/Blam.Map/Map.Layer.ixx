export module Map.Layer;

import Common.Log.System;
import Map.Reader.State;
import Map.Reader.System;
import Map.Tag.Type;
import Map.Tag.State;
import Map.Builder.System;

export namespace Map
{
    // Root of the Map layer. Owns the stores and services that read the '.map' file and fill the tag catalog.
    // The map is loaded when the owner reports the file the engine opened (MapLoaderService::OpenFile) and cleaned up with Cleanup.
    // note: Members are built in declaration order, so stores come first, then services. Pure: it knows nothing of the game
    // process, and the owner connects the lifecycle (D178).
    class Layer
    {
    private:
        using Log = Common::Log::System::Log;
        using TagCatalog = Map::Tag::State::TagCatalog;

        using FileStore = Map::Reader::State::FileStore;
        using TagIndexStore = Map::Reader::State::TagIndexStore;
        using FormulaService = Map::Reader::System::FormulaService;
        using DataStreamService = Map::Reader::System::DataStreamService;
        using FileLocatorService = Map::Reader::System::FileLocatorService;
        using HeaderService = Map::Reader::System::HeaderService;
        using TagIndexService = Map::Reader::System::TagIndexService;
        using FileNamesService = Map::Reader::System::FileNamesService;
        using TagReferenceService = Map::Reader::System::TagReferenceService;
        using TagStructureService = Map::Reader::System::TagStructureService;
        using GeometryInfoService = Map::Reader::System::GeometryInfoService;
        using GeometryMeshDecoderService = Map::Reader::System::GeometryMeshDecoderService;
        using GeometryResourcePageService = Map::Reader::System::GeometryResourcePageService;
        using GeometryLoaderService = Map::Reader::System::GeometryLoaderService;
        using MapLoaderService = Map::Reader::System::MapLoaderService;

        using MapBuilderService = Map::Builder::System::MapBuilderService;

    public:
        explicit Layer(Log& log) :
            m_FileLocatorService(m_FileStore),
            m_HeaderService(m_FormulaService),
            m_TagIndexService(m_TagIndexStore, m_FormulaService),
            m_FileNamesService(m_TagIndexStore, m_FormulaService),
            m_TagReferenceService(m_TagIndexStore, m_FormulaService),
            m_TagStructureService(m_TagIndexStore),
            m_GeometryInfoService(m_FileStore, m_DataStreamService),
            m_GeometryMeshDecoderService(log, m_FileStore, m_DataStreamService, m_FormulaService),
            m_GeometryResourcePageService(log, m_FileStore, m_FileLocatorService, m_DataStreamService, m_FormulaService),
            m_GeometryLoaderService(log, m_TagCatalog, m_GeometryResourcePageService, m_GeometryInfoService, m_GeometryMeshDecoderService, m_FormulaService),
            m_MapLoaderService(log, m_FileStore, m_TagIndexStore, m_FileLocatorService, m_FileNamesService, m_HeaderService, m_TagIndexService),
            m_MapBuilderService(log, m_TagCatalog, m_FileStore, m_TagIndexStore, m_FileLocatorService, m_TagReferenceService) {}
        ~Layer() = default;

        // Not copyable. Members hold references to each other.
        Layer(const Layer&) = delete;
        auto operator=(const Layer&) -> Layer& = delete;

        // --- State ---
        TagCatalog m_TagCatalog{};
        FileStore m_FileStore{};
        TagIndexStore m_TagIndexStore{};

        // --- System ---
        FormulaService m_FormulaService{};
        DataStreamService m_DataStreamService{};
        FileLocatorService m_FileLocatorService;
        HeaderService m_HeaderService;
        TagIndexService m_TagIndexService;
        FileNamesService m_FileNamesService;
        TagReferenceService m_TagReferenceService;
        TagStructureService m_TagStructureService;
        GeometryInfoService m_GeometryInfoService;
        GeometryMeshDecoderService m_GeometryMeshDecoderService;
        GeometryResourcePageService m_GeometryResourcePageService;
        GeometryLoaderService m_GeometryLoaderService;
        MapLoaderService m_MapLoaderService;
        MapBuilderService m_MapBuilderService;

        // Clears the map, the built catalog and the formula state, so the next map starts from scratch.
        auto Cleanup() -> void
        {
            m_MapLoaderService.Cleanup();
            m_MapBuilderService.Cleanup();
            m_FormulaService.Cleanup();
        }
    };
}