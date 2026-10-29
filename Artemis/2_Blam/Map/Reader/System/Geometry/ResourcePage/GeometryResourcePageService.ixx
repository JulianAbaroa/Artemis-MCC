export module Blam.Map.Reader.System:Geometry.ResourcePage;

import :FileLocator;
import :DataStream;
import :Formula;

import Common.Log.System;
import Blam.Map.Reader.State;
import Blam.Map.Tag.Type;
import std;

export namespace Blam::Map::Reader::System
{
    // Reads the raw page that holds the buffers of a geometry resource.
    class GeometryResourcePageService
    {
    private:
        using ZoneObject = Blam::Map::Tag::Type::Zone::Object::ZoneObject;
        using Zone_TagResourcesObject = Blam::Map::Tag::Type::Zone::Object::Zone_TagResourcesObject;
        using PlayObject = Blam::Map::Tag::Type::Play::Object::PlayObject;

        using Log = Common::Log::System::Log;
        using FileStore = Blam::Map::Reader::State::FileStore;
        using FileLocatorService = Blam::Map::Reader::System::FileLocatorService;
        using DataStreamService = Blam::Map::Reader::System::DataStreamService;
        using FormulaService = Blam::Map::Reader::System::FormulaService;

    public:
        GeometryResourcePageService(Log& log, FileStore& fileStore,
            FileLocatorService& fileLocatorService,
            DataStreamService& dataStreamService,
            FormulaService& formulaService) :
            m_Log(log), m_FileStore(fileStore),
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
        auto Read(std::uint32_t datum, const Zone_TagResourcesObject** outEntry,
            const ZoneObject* zone, const PlayObject* play) const
            -> std::vector<std::uint8_t>;

    private:
        Log& m_Log;
        FileStore& m_FileStore;
        FileLocatorService& m_FileLocatorService;
        DataStreamService& m_DataStreamService;
        FormulaService& m_FormulaService;
    };
}