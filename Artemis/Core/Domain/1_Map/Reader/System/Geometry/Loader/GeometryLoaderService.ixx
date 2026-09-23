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
		using SbspObject = Map::Tag::Type::Sbsp::Object::SbspObject;
		using LbspObject = Map::Tag::Type::Lbsp::Object::LbspObject;
		using PlayObject = Map::Tag::Type::Play::Object::PlayObject;
		using ZoneObject = Map::Tag::Type::Zone::Object::ZoneObject;
		using SbspGeometry = Map::Reader::Type::Geometry::SbspGeometry;

		using LogsService = Service::Logs::System::LogsService;
		template <typename TObject> using SbspStore = Map::Tag::State::Sbsp::SbspStore<TObject>;
		template <typename TObject> using LbspStore = Map::Tag::State::Lbsp::LbspStore<TObject>;
		template <typename TObject> using PlayStore = Map::Tag::State::Play::PlayStore<TObject>;
		template <typename TObject> using ZoneStore = Map::Tag::State::Zone::ZoneStore<TObject>;
		using FormulaService = Map::Reader::System::FormulaService;
		using GeometryInfoService = Map::Reader::System::GeometryInfoService;
		using GeometryResourcePageService = Map::Reader::System::GeometryResourcePageService;
		using GeometryMeshDecoderService = Map::Reader::System::GeometryMeshDecoderService;

	public:
		GeometryLoaderService(LogsService& logsService, 
			SbspStore<SbspObject>& sbspStore, LbspStore<LbspObject>& lbspStore, 
			PlayStore<PlayObject>& playStore, ZoneStore<ZoneObject>& zoneStore,
			GeometryResourcePageService& geometryResourcePageService,
			GeometryInfoService& geometryInfoService,
			GeometryMeshDecoderService& geometryMeshDecoderService,
			FormulaService& m_FormulaService) :
			m_LogsService(logsService), m_SbspStore(sbspStore), 
			m_LbspStore(lbspStore),  m_PlayStore(playStore), m_ZoneStore(zoneStore), 
			m_GeometryResourcePageService(geometryResourcePageService),
			m_GeometryInfoService(geometryInfoService), 
			m_GeometryMeshDecoderService(geometryMeshDecoderService),
			m_FormulaService(m_FormulaService) {}
		~GeometryLoaderService() = default;

		auto ReadRenderGeometry(std::vector<std::string>& sbspTagNames) const 
			-> std::vector<SbspGeometry>;

	private:
		LogsService& m_LogsService;
		SbspStore<SbspObject>& m_SbspStore;
		LbspStore<LbspObject>& m_LbspStore;
		PlayStore<PlayObject>& m_PlayStore;
		ZoneStore<ZoneObject>& m_ZoneStore;
		GeometryInfoService& m_GeometryInfoService;
		FormulaService& m_FormulaService;
		GeometryMeshDecoderService& m_GeometryMeshDecoderService;
		GeometryResourcePageService& m_GeometryResourcePageService;
	};
}