// This geometry extraction logic is derived from Reclaimer
// (https://github.com/Gravemind2401/Reclaimer) by Gravemind2401,
// licensed under GPL-3.0. The mesh-access theory, buffer layout,
// unstrip algorithm and instance transform handling were ported
// from Reclaimer's C# implementation to C++.
// See MeshReader.md for the full Assembly <-> Reclaimer name mapping.

export module Blam.Map.Reader.System:Geometry.Loader;

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
    // Builds the render geometry of the structure BSPs: the world meshes of each cluster plus the instanced geometry.
    class GeometryLoaderService
    {
    private:
        using ZoneObject = Blam::Map::Tag::Type::Zone::Object::ZoneObject;
        using PlayObject = Blam::Map::Tag::Type::Play::Object::PlayObject;
        using SbspObject = Blam::Map::Tag::Type::Sbsp::Object::SbspObject;
        using LbspObject = Blam::Map::Tag::Type::Lbsp::Object::LbspObject;
        using SbspGeometry = Blam::Map::Reader::Type::Geometry::SbspGeometry;

        using Log = Common::Log::System::Log;
        using TagCatalog = Blam::Map::Tag::State::TagCatalog;
        using FormulaService = Blam::Map::Reader::System::FormulaService;
        using GeometryInfoService = Blam::Map::Reader::System::GeometryInfoService;
        using GeometryResourcePageService = Blam::Map::Reader::System::GeometryResourcePageService;
        using GeometryMeshDecoderService = Blam::Map::Reader::System::GeometryMeshDecoderService;

    public:
        GeometryLoaderService(Log& log, TagCatalog& tagCatalog,
            GeometryResourcePageService& geometryResourcePageService,
            GeometryInfoService& geometryInfoService,
            GeometryMeshDecoderService& geometryMeshDecoderService,
            FormulaService& formulaService) :
            m_Log(log), m_TagCatalog(tagCatalog),
            m_GeometryResourcePageService(geometryResourcePageService),
            m_GeometryInfoService(geometryInfoService),
            m_GeometryMeshDecoderService(geometryMeshDecoderService),
            m_FormulaService(formulaService) {}
        ~GeometryLoaderService() = default;

        // Decodes the geometry of each structure BSP that has a lightmap BSP with meshes.
        // param sbspTagNames: Names of the sbsp tags to decode.
        // return: One geometry per decoded BSP. Empty if the zone or play tags are missing.
        auto ReadRenderGeometry(std::vector<std::string>& sbspTagNames) const
            -> std::vector<SbspGeometry>;

    private:
        Log& m_Log;
        TagCatalog& m_TagCatalog;
        GeometryResourcePageService& m_GeometryResourcePageService;
        GeometryInfoService& m_GeometryInfoService;
        GeometryMeshDecoderService& m_GeometryMeshDecoderService;
        FormulaService& m_FormulaService;
    };
}