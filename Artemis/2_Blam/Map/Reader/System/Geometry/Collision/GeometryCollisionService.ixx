export module Blam.Map.Reader.System:Geometry.Collision;

import :Formula;
import :Geometry.Info;
import :Geometry.MeshDecoder;
import :Geometry.ResourcePage;

import Common.Log.System;
import Blam.Map.Reader.Type;
import Blam.Map.Tag.Type;
import Blam.Map.Tag.State;
import std;

export namespace Blam::Map::Reader::System
{
    // Reads the collision of the structure BSPs: the static surfaces and the instanced geometry definitions with their placements.
    // note: The collision of a BSP lives in a tag resource (type 5) whose definition holds the tag blocks and whose raw page holds
    // their elements. The structure uses the normal or the large element layout, the instanced geometry always the normal one (D272).
    class GeometryCollisionService
    {
    private:
        using ZoneObject = Blam::Map::Tag::Type::Zone::Object::ZoneObject;
        using PlayObject = Blam::Map::Tag::Type::Play::Object::PlayObject;
        using SbspObject = Blam::Map::Tag::Type::Sbsp::Object::SbspObject;
        using SbspCollision = Blam::Map::Reader::Type::Geometry::SbspCollision;

        using Log = Common::Log::System::Log;
        using TagCatalog = Blam::Map::Tag::State::TagCatalog;
        using FormulaService = Blam::Map::Reader::System::FormulaService;
        using GeometryInfoService = Blam::Map::Reader::System::GeometryInfoService;
        using GeometryResourcePageService = Blam::Map::Reader::System::GeometryResourcePageService;
        using GeometryMeshDecoderService = Blam::Map::Reader::System::GeometryMeshDecoderService;

    public:
        GeometryCollisionService(Log& log, TagCatalog& tagCatalog,
            GeometryResourcePageService& geometryResourcePageService,
            GeometryInfoService& geometryInfoService,
            GeometryMeshDecoderService& geometryMeshDecoderService,
            FormulaService& formulaService) :
            m_Log(log), m_TagCatalog(tagCatalog),
            m_GeometryResourcePageService(geometryResourcePageService),
            m_GeometryInfoService(geometryInfoService),
            m_GeometryMeshDecoderService(geometryMeshDecoderService),
            m_FormulaService(formulaService) {}
        ~GeometryCollisionService() = default;

        // Reads the collision of each structure BSP.
        // param sbspTagNames: Names of the sbsp tags to read.
        // return: One collision per BSP whose resource could be read. A BSP with inline collision or without resource is
        // skipped with a warning or an info. Empty if the zone or play tags are missing.
        auto ReadCollision(const std::vector<std::string>& sbspTagNames) const -> std::vector<SbspCollision>;

    private:
        Log& m_Log;
        TagCatalog& m_TagCatalog;
        GeometryResourcePageService& m_GeometryResourcePageService;
        GeometryInfoService& m_GeometryInfoService;
        GeometryMeshDecoderService& m_GeometryMeshDecoderService;
        FormulaService& m_FormulaService;
    };
}