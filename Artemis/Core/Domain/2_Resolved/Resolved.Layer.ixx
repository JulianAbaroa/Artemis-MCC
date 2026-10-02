export module Resolved.Layer;

import Service.Layer;
import Platform.Layer;
import Map.Layer;
import Resolved.Definitions.State;
import Resolved.Definitions.System;
import Resolved.World.State;
import Resolved.World.System;
import Resolved.Vitality.State;
import Resolved.Vitality.System;

export namespace Resolved
{
	class Layer
	{
	private:
		using WorldStore = Resolved::World::State::WorldStore;
		using DefinitionsStore = Resolved::Definitions::State::DefinitionsStore;
		using VitalityStore = Resolved::Vitality::State::VitalityStore;

		using ObjectBuilder = Resolved::Definitions::System::ObjectBuilder;
		using BipdBuilder = Resolved::Definitions::System::BipdBuilder;
		using BlocBuilder = Resolved::Definitions::System::BlocBuilder;
		using CollBuilder = Resolved::Definitions::System::CollBuilder;
		using CtrlBuilder = Resolved::Definitions::System::CtrlBuilder;
		using EqipBuilder = Resolved::Definitions::System::EqipBuilder;
		using HlmtBuilder = Resolved::Definitions::System::HlmtBuilder;
		using JptBuilder = Resolved::Definitions::System::JptBuilder;
		using MachBuilder = Resolved::Definitions::System::MachBuilder;
		using ProjBuilder = Resolved::Definitions::System::ProjBuilder;
		using SbspBuilder = Resolved::Definitions::System::SbspBuilder;
		using ScenBuilder = Resolved::Definitions::System::ScenBuilder;
		using ScnrBuilder = Resolved::Definitions::System::ScnrBuilder;
		using WeapBuilder = Resolved::Definitions::System::WeapBuilder;
		using VehiBuilder = Resolved::Definitions::System::VehiBuilder;
		using DefinitionsBuilder = Resolved::Definitions::System::DefinitionsBuilder;

		using ModeBuilder = Resolved::Definitions::System::ModeBuilder;
		using RegionStatesBuilder = Resolved::World::System::RegionStatesBuilder;
		using WorldBuilder = Resolved::World::System::WorldBuilder;
		using SbspRaycaster = Resolved::World::System::SbspRaycaster;
		using ModelLinkBuilder = Resolved::World::System::ModelLinkBuilder;

		using VitalityBuilder = Resolved::Vitality::System::VitalityBuilder;

	public:
		Layer(Service::Layer& service, Platform::Layer& platform, Map::Layer& map) :
			m_ObjectBuilder(map.m_TagResolverService),
			m_BipdBuilder(m_ObjectBuilder),
			m_BlocBuilder(m_ObjectBuilder),
			m_CtrlBuilder(m_ObjectBuilder),
			m_EqipBuilder(m_ObjectBuilder),
			m_HlmtBuilder(map.m_TagResolverService),
			m_MachBuilder(m_ObjectBuilder),
			m_ProjBuilder(m_ObjectBuilder),
			m_ScenBuilder(m_ObjectBuilder),
			m_WeapBuilder(m_ObjectBuilder),
			m_VehiBuilder(m_ObjectBuilder),
			m_DefinitionsBuilder(service.m_LogsService, map.m_TagIndexStore, map.m_TagCatalog, map.m_GeometryLoaderService, m_DefinitionsStore, m_BipdBuilder, m_BlocBuilder, m_CollBuilder, m_CtrlBuilder, m_EqipBuilder, m_HlmtBuilder, m_JptBuilder, m_MachBuilder, m_ModeBuilder, m_ProjBuilder, m_SbspBuilder, m_ScenBuilder, m_ScnrBuilder, m_VehiBuilder, m_WeapBuilder),
			m_WorldBuilder(service.m_LogsService, map.m_TagCatalog, m_WorldStore, m_DefinitionsStore, map.m_TagResolverService, m_RegionStatesBuilder, m_ModelLinkBuilder),
			m_VitalityBuilder(service.m_LogsService, m_DefinitionsStore, m_WorldStore, m_VitalityStore)
		{
			platform.m_LifecycleService.OnCleanup([this] {
				m_WorldBuilder.Cleanup();
				m_DefinitionsBuilder.Cleanup();
				m_VitalityBuilder.Cleanup();
				m_SbspRaycaster.Cleanup();
			});
		}
		~Layer() = default;

		Layer(const Layer&) = delete;
		Layer& operator=(const Layer&) = delete;

		// --- State ---
		DefinitionsStore m_DefinitionsStore;
		VitalityStore m_VitalityStore;
		WorldStore m_WorldStore;

		// --- System: Definitions ---
		ObjectBuilder m_ObjectBuilder;
		BipdBuilder m_BipdBuilder;
		BlocBuilder m_BlocBuilder;
		CollBuilder m_CollBuilder;
		CtrlBuilder m_CtrlBuilder;
		EqipBuilder m_EqipBuilder;
		HlmtBuilder m_HlmtBuilder;
		JptBuilder m_JptBuilder;
		MachBuilder m_MachBuilder;
		ModeBuilder m_ModeBuilder;
		WeapBuilder m_WeapBuilder;
		VehiBuilder m_VehiBuilder;
		ProjBuilder m_ProjBuilder;
		SbspBuilder m_SbspBuilder;
		ScenBuilder m_ScenBuilder;
		ScnrBuilder m_ScnrBuilder;
		DefinitionsBuilder m_DefinitionsBuilder;

		// --- System: World ---
		RegionStatesBuilder m_RegionStatesBuilder;
		WorldBuilder m_WorldBuilder;
		SbspRaycaster m_SbspRaycaster{};
		ModelLinkBuilder m_ModelLinkBuilder;

		// --- System: Vitality ---
		VitalityBuilder m_VitalityBuilder;
	};
}