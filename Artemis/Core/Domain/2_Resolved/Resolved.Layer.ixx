export module Resolved.Layer;

import Service.Layer;
import Platform.Layer;
import Map.Layer;
import Resolved.World.State;
import Resolved.World.System;
import Resolved.Stats.State;
import Resolved.Stats.System;
import Resolved.Vitality.State;
import Resolved.Vitality.System;

export namespace Resolved
{
	class Layer
	{
	private:
		using WorldStore = Resolved::World::State::WorldStore;
		using StatsStore = Resolved::Stats::State::StatsStore;
		using VitalityStore = Resolved::Vitality::State::VitalityStore;

		using WeapBuilder = Resolved::Stats::System::WeapBuilder;
		using VehiBuilder = Resolved::Stats::System::VehiBuilder;
		using ProjBuilder = Resolved::Stats::System::ProjBuilder;
		using StatsBuilder = Resolved::Stats::System::StatsBuilder;

		using CollBuilder = Resolved::World::System::CollBuilder;
		using ModeBuilder = Resolved::World::System::ModeBuilder;
		using PhmoBuilder = Resolved::World::System::PhmoBuilder;
		using SbspBuilder = Resolved::World::System::SbspBuilder;
		using SbspSeamLinker = Resolved::World::System::SbspSeamLinker;
		using RegionStatesBuilder = Resolved::World::System::RegionStatesBuilder;
		using WorldBuilder = Resolved::World::System::WorldBuilder;

		using VitalityBuilder = Resolved::Vitality::System::VitalityBuilder;

	public:
		Layer(Service::Layer& service, Platform::Layer& platform, Map::Layer& map) :
			m_StatsBuilder(service.m_LogsService, map.m_TagIndexStore, map.m_TagStore.proj, map.m_TagStore.vehi, map.m_TagStore.weap, m_StatsStore, m_ProjBuilder, m_VehiBuilder, m_WeapBuilder),
			m_WorldBuilder(service.m_LogsService, map.m_TagIndexStore, map.m_TagStore.bipd, map.m_TagStore.bloc, map.m_TagStore.coll, map.m_TagStore.ctrl, map.m_TagStore.eqip, map.m_TagStore.hlmt, map.m_TagStore.mach, map.m_TagStore.mode, map.m_TagStore.sbsp, map.m_TagStore.scen, map.m_TagStore.vehi, map.m_TagStore.weap, m_WorldStore, map.m_TagResolverService, map.m_GeometryLoaderService, m_SbspSeamLinker, m_SbspBuilder, m_CollBuilder, m_ModeBuilder, m_RegionStatesBuilder),
			m_VitalityBuilder(service.m_LogsService, map.m_TagStore.coll, map.m_TagStore.hlmt, m_VitalityStore)
		{
			platform.m_LifecycleService.OnCleanup([this] {
				m_WorldBuilder.Cleanup();
				m_StatsBuilder.Cleanup();
				m_VitalityBuilder.Cleanup();
			});
		}
		~Layer() = default;

		Layer(const Layer&) = delete;
		Layer& operator=(const Layer&) = delete;

		// --- State ---
		WorldStore m_WorldStore;
		StatsStore m_StatsStore;
		VitalityStore m_VitalityStore;

		// --- System: Stats ---
		WeapBuilder m_WeapBuilder;
		VehiBuilder m_VehiBuilder;
		ProjBuilder m_ProjBuilder;
		StatsBuilder m_StatsBuilder;

		// --- System: World ---
		CollBuilder m_CollBuilder;
		ModeBuilder m_ModeBuilder;
		PhmoBuilder m_PhmoBuilder;
		SbspBuilder m_SbspBuilder;
		SbspSeamLinker m_SbspSeamLinker;
		RegionStatesBuilder m_RegionStatesBuilder;
		WorldBuilder m_WorldBuilder;

		// --- System: Vitality ---
		VitalityBuilder m_VitalityBuilder;
	};
}