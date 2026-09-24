// This geometry extraction logic is derived from Reclaimer
// (https://github.com/Gravemind2401/Reclaimer) by Gravemind2401,
// licensed under GPL-3.0. The mesh-access theory, buffer layout,
// unstrip algorithm and instance transform handling were ported
// from Reclaimer's C# implementation to C++.
// See MeshReader.md for the full Assembly <-> Reclaimer name mapping.

export module Map.Reader.System:Geometry.Loader;

import :Formula;
import :Geometry.Info;
import :Geometry.MeshDecoder;
import :Geometry.ResourcePage;
import Service.Logs.System;
import Map.Reader.Type;
import Map.Tag.Type;
import Map.Tag.State;
import std;

export namespace Map::Reader::System
{
	class GeometryLoaderService
	{
	private:
		using ZoneObject = Map::Tag::Type::Zone::Object::ZoneObject;
		using PlayObject = Map::Tag::Type::Play::Object::PlayObject;
		using SbspObject = Map::Tag::Type::Sbsp::Object::SbspObject;
		using LbspObject = Map::Tag::Type::Lbsp::Object::LbspObject;
		using SbspGeometry = Map::Reader::Type::Geometry::SbspGeometry;

		using LogsService = Service::Logs::System::LogsService;
		using TagCatalog = Map::Tag::State::TagCatalog;
		using FormulaService = Map::Reader::System::FormulaService;
		using GeometryInfoService = Map::Reader::System::GeometryInfoService;
		using GeometryResourcePageService = Map::Reader::System::GeometryResourcePageService;
		using GeometryMeshDecoderService = Map::Reader::System::GeometryMeshDecoderService;

	public:
		GeometryLoaderService(LogsService& logsService, TagCatalog& tagCatalog,
			GeometryResourcePageService& geometryResourcePageService,
			GeometryInfoService& geometryInfoService,
			GeometryMeshDecoderService& geometryMeshDecoderService,
			FormulaService& m_FormulaService) :
			m_LogsService(logsService), m_TagCatalog(tagCatalog),
			m_GeometryResourcePageService(geometryResourcePageService),
			m_GeometryInfoService(geometryInfoService), 
			m_GeometryMeshDecoderService(geometryMeshDecoderService),
			m_FormulaService(m_FormulaService) {}
		~GeometryLoaderService() = default;

		auto ReadRenderGeometry(std::vector<std::string>& sbspTagNames) const 
			-> std::vector<SbspGeometry>;

	private:
		LogsService& m_LogsService;
		TagCatalog& m_TagCatalog;
		GeometryInfoService& m_GeometryInfoService;
		FormulaService& m_FormulaService;
		GeometryMeshDecoderService& m_GeometryMeshDecoderService;
		GeometryResourcePageService& m_GeometryResourcePageService;
	};
}