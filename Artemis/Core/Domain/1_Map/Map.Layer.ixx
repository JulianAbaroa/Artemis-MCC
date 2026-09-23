export module Map.Layer;

import Service.Layer;
import Platform.Layer;
import Map.Reader.State;
import Map.Reader.System;
import Map.Reader.Hook;
import Map.Tag.State;
import Map.Tag.Type;
import Map.Builder.System;

export namespace Map
{
	struct TagStores
	{
		Tag::State::Bipd::BipdStore<Tag::Type::Bipd::Object::BipdObject> bipd;
		Tag::State::Bloc::BlocStore<Tag::Type::Bloc::Object::BlocObject> bloc;
		Tag::State::Coll::CollStore<Tag::Type::Coll::Object::CollObject> coll;
		Tag::State::Ctrl::CtrlStore<Tag::Type::Ctrl::Object::CtrlObject> ctrl;
		Tag::State::Eqip::EqipStore<Tag::Type::Eqip::Object::EqipObject> eqip;
		Tag::State::Hlmt::HlmtStore<Tag::Type::Hlmt::Object::HlmtObject> hlmt;
		Tag::State::Lbsp::LbspStore<Tag::Type::Lbsp::Object::LbspObject> lbsp;
		Tag::State::Mach::MachStore<Tag::Type::Mach::Object::MachObject> mach;
		Tag::State::Mode::ModeStore<Tag::Type::Mode::Object::ModeObject> mode;
		Tag::State::Phmo::PhmoStore<Tag::Type::Phmo::Object::PhmoObject> phmo;
		Tag::State::Play::PlayStore<Tag::Type::Play::Object::PlayObject> play;
		Tag::State::Proj::ProjStore<Tag::Type::Proj::Object::ProjObject> proj;
		Tag::State::Sbsp::SbspStore<Tag::Type::Sbsp::Object::SbspObject> sbsp;
		Tag::State::Scen::ScenStore<Tag::Type::Scen::Object::ScenObject> scen;
		Tag::State::Scnr::ScnrStore<Tag::Type::Scnr::Object::ScnrObject> scnr;
		Tag::State::Sldt::SldtStore<Tag::Type::Sldt::Object::SldtObject> sldt;
		Tag::State::Vehi::VehiStore<Tag::Type::Vehi::Object::VehiObject> vehi;
		Tag::State::Weap::WeapStore<Tag::Type::Weap::Object::WeapObject> weap;
		Tag::State::Zone::ZoneStore<Tag::Type::Zone::Object::ZoneObject> zone;
	};

	class Layer
	{
	private:
		using FileStore = Map::Reader::State::FileStore;
		using TagIndexStore = Map::Reader::State::TagIndexStore;

		using FormulaService = Map::Reader::System::FormulaService;
		using DataStreamService = Map::Reader::System::DataStreamService;
		using FileLocatorService = Map::Reader::System::FileLocatorService;
		using HeaderService = Map::Reader::System::HeaderService;
		using TagIndexService = Map::Reader::System::TagIndexService;
		using FileNamesService = Map::Reader::System::FileNamesService;
		using TagResolverService = Map::Reader::System::TagResolverService;
		using TagStructureService = Map::Reader::System::TagStructureService;
		using GeometryInfoService = Map::Reader::System::GeometryInfoService;
		using GeometryMeshDecoderService = Map::Reader::System::GeometryMeshDecoderService;
		using GeometryResourcePageService = Map::Reader::System::GeometryResourcePageService;
		using GeometryLoaderService = Map::Reader::System::GeometryLoaderService;
		using MapLoaderService = Map::Reader::System::MapLoaderService;
		using MapBuilderService = Map::Builder::System::MapBuilderService;

		using OpenMapDetour = Map::Reader::Hook::OpenMapDetour;

	public:
		Layer(Service::Layer& service, Platform::Layer& platform) :
			m_FileLocatorService(m_FileStore),
			m_HeaderService(m_FormulaService),
			m_TagIndexService(m_TagIndexStore, m_FormulaService),
			m_FileNamesService(m_TagIndexStore, m_FormulaService),
			m_TagResolverService(service.m_LogsService, m_FileStore, m_TagIndexStore, m_FormulaService),
			m_TagStructureService(m_TagIndexStore),
			m_GeometryInfoService(m_FileStore, m_DataStreamService),
			m_GeometryMeshDecoderService(service.m_LogsService, m_FileStore, m_DataStreamService, m_FormulaService),
			m_GeometryResourcePageService(service.m_LogsService, m_FileStore, m_FileLocatorService, m_DataStreamService, m_FormulaService),
			m_GeometryLoaderService(service.m_LogsService, m_TagStore.sbsp, m_TagStore.lbsp, m_TagStore.play, m_TagStore.zone, m_GeometryResourcePageService, m_GeometryInfoService, m_GeometryMeshDecoderService, m_FormulaService),
			m_MapLoaderService(service.m_LogsService, m_FileStore, m_TagIndexStore, m_FileLocatorService, m_FileNamesService, m_HeaderService, m_TagIndexService),
			m_MapBuilderService(service.m_LogsService, m_TagStore.bipd, m_TagStore.bloc, m_TagStore.coll, m_TagStore.ctrl, m_TagStore.eqip, m_TagStore.hlmt, m_TagStore.lbsp, m_TagStore.mach, m_TagStore.mode, m_TagStore.phmo, m_TagStore.play, m_TagStore.proj, m_TagStore.sbsp, m_TagStore.scen, m_TagStore.scnr, m_TagStore.sldt, m_TagStore.vehi, m_TagStore.weap, m_TagStore.zone, m_FileStore, m_TagIndexStore, m_FileLocatorService, m_TagResolverService),
			m_OpenMapDetour(service.m_LogsService, platform.m_AOBService, m_FileStore, m_MapLoaderService)
		{
			auto& lifecycle = platform.m_LifecycleService;

			lifecycle.OnEngineInitialized([this] {
				m_OpenMapDetour.Install();
			});

			lifecycle.OnUnhook([this] {
				m_OpenMapDetour.Uninstall();
			});

			lifecycle.OnCleanup([this] {
				m_MapLoaderService.Cleanup();
				m_MapBuilderService.Cleanup();
				m_FormulaService.Cleanup();
			});
		}
		~Layer() = default;

		Layer(const Layer&) = delete;
		Layer& operator=(const Layer&) = delete;

		// --- State ---
		TagStores m_TagStore;
		FileStore m_FileStore;
		TagIndexStore m_TagIndexStore;

		// --- System: Reader ---
		FormulaService m_FormulaService;
		DataStreamService m_DataStreamService;
		FileLocatorService m_FileLocatorService;
		HeaderService m_HeaderService;
		TagIndexService m_TagIndexService;
		FileNamesService m_FileNamesService;
		TagResolverService m_TagResolverService;
		TagStructureService m_TagStructureService;
		GeometryInfoService m_GeometryInfoService;
		GeometryMeshDecoderService m_GeometryMeshDecoderService;
		GeometryResourcePageService m_GeometryResourcePageService;
		GeometryLoaderService m_GeometryLoaderService;
		MapLoaderService m_MapLoaderService;

		// --- System: Builder ---
		MapBuilderService m_MapBuilderService;

		// --- Hook ---
		OpenMapDetour m_OpenMapDetour;
	};
}