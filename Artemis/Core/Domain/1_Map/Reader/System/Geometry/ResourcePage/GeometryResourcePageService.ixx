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
    // Reads the raw page that holds the buffers of a geometry resource.
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
            FormulaService& formulaService) :
            m_LogsService(logsService), m_FileStore(fileStore),
            m_FileLocatorService(fileLocatorService),
            m_DataStreamService(dataStreamService),
            m_FormulaService(formulaService) {}
        ~GeometryResourcePageService() = default;

        // Reads the page of a resource from this map, or from an external cache map if the page lives there.
        // param datum: Datum of the resource, as stored in the zone asset field.
        // param outEntry: Out. Receives the tag resource of the datum. Can be null.
        // param zone: Zone tag that lists the resources.
        // param play: Play tag that lists the segments and pages.
        // return: The bytes of the resource, or empty if an index is out of range or the page cannot be read.
        auto Read(std::uint32_t datum, const TagResourcesObject** outEntry,
            const ZoneObject* zone, const PlayObject* play) const
            -> std::vector<std::uint8_t>;

    private:
        LogsService& m_LogsService;
        FileStore& m_FileStore;
        FileLocatorService& m_FileLocatorService;
        DataStreamService& m_DataStreamService;
        FormulaService& m_FormulaService;
    };
}