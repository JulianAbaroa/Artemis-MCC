export module Environment.Layer;

import Service.Layer;
import Platform.Layer;
import Resolved.Layer;
import Tables.Layer;
import Relations.Layer;
import Environment.Details.State;
import Environment.Details.System;
import Environment.Entities.State;
import Environment.Entities.System;
import Environment.Players.State;
import Environment.Players.System;
import Environment.Collidable.State;
import Environment.Collidable.System;
import Environment.Aim.State;
import Environment.Aim.System;

export namespace Environment
{
	class Layer
	{
	private:
		using VitalityDetailsStore = Environment::Details::State::VitalityDetailsStore;
		using ZoneDetailsStore = Environment::Details::State::ZoneDetailsStore;
		using TeleporterDetailsStore = Environment::Details::State::TeleporterDetailsStore;
		using LiftDetailsStore = Environment::Details::State::LiftDetailsStore;
		using BarrierDetailsStore = Environment::Details::State::BarrierDetailsStore;
		using EntitiesStore = Environment::Entities::State::EntitiesStore;
		using CarriedHandlesStore = Environment::Entities::State::CarriedHandlesStore;
		using PlayersStore = Environment::Players::State::PlayersStore;
		using CollidableStore = Environment::Collidable::State::CollidableStore;
		using AimStore = Environment::Aim::State::AimStore;

		using VitalityDetailsService = Environment::Details::System::VitalityDetailsService;
		using ZoneDetailsService = Environment::Details::System::ZoneDetailsService;
		using TeleporterDetailsService = Environment::Details::System::TeleporterDetailsService;
		using LiftDetailsService = Environment::Details::System::LiftDetailsService;
		using BarrierDetailsService = Environment::Details::System::BarrierDetailsService;
		using EntitiesService = Environment::Entities::System::EntitiesService;
		using PlayersService = Environment::Players::System::PlayersService;
		using CollidableService = Environment::Collidable::System::CollidableService;
		using AimService = Environment::Aim::System::AimService;

	public:
		Layer(Service::Layer& service, Platform::Layer& platform, Resolved::Layer& resolved,
			Tables::Layer& tables, Relations::Layer& relations) :
			m_VitalityDetailsService(service.m_LogsService, resolved.m_VitalityStore, tables.m_ObjectStore, tables.m_DamageSectionsStore, relations.m_ClassifierStore, m_VitalityDetailsStore),
			m_ZoneDetailsService(service.m_LogsService, tables.m_ObjectStore, relations.m_ClassifierStore, m_ZoneDetailsStore),
			m_TeleporterDetailsService(service.m_LogsService, tables.m_ObjectStore, relations.m_ClassifierStore, m_TeleporterDetailsStore),
			m_LiftDetailsService(service.m_LogsService, tables.m_ObjectStore, relations.m_ClassifierStore, m_LiftDetailsStore),
			m_BarrierDetailsService(service.m_LogsService, tables.m_ObjectStore, relations.m_ClassifierStore, m_BarrierDetailsStore),
			m_EntitiesService(service.m_LogsService, tables.m_ObjectStore, tables.m_PlayerStore, relations.m_ClassifierStore, m_VitalityDetailsStore, m_EntitiesStore, m_CarriedHandlesStore),
			m_PlayersService(service.m_LogsService, tables.m_ObjectStore, tables.m_PlayerStore, m_VitalityDetailsStore, m_EntitiesStore, m_PlayersStore),
			m_CollidableService(service.m_LogsService, resolved.m_DefinitionsStore, tables.m_ObjectStore, tables.m_BoneMatricesStore, tables.m_DamageSectionsStore, relations.m_ClassifierStore, resolved.m_WorldStore, m_CollidableStore),
			m_AimService(service.m_LogsService, resolved.m_VitalityStore, tables.m_ObjectStore, tables.m_BoneMatricesStore, m_AimStore)
		{
			platform.m_LifecycleService.OnCleanup([this] {
				m_VitalityDetailsService.Cleanup();
				m_ZoneDetailsService.Cleanup();
				m_TeleporterDetailsService.Cleanup();
				m_LiftDetailsService.Cleanup();
				m_BarrierDetailsService.Cleanup();
				m_EntitiesService.Cleanup();
				m_PlayersService.Cleanup();
				m_CollidableService.Cleanup();
				m_AimService.Cleanup();
			});
		}
		~Layer() = default;

		Layer(const Layer&) = delete;
		Layer& operator=(const Layer&) = delete;

		// --- State ---
		VitalityDetailsStore m_VitalityDetailsStore;
		ZoneDetailsStore m_ZoneDetailsStore;
		TeleporterDetailsStore m_TeleporterDetailsStore;
		LiftDetailsStore m_LiftDetailsStore;
		BarrierDetailsStore m_BarrierDetailsStore;
		EntitiesStore m_EntitiesStore;
		CarriedHandlesStore m_CarriedHandlesStore;
		PlayersStore m_PlayersStore;
		CollidableStore m_CollidableStore;
		AimStore m_AimStore;

		// --- System ---
		VitalityDetailsService m_VitalityDetailsService;
		ZoneDetailsService m_ZoneDetailsService;
		TeleporterDetailsService m_TeleporterDetailsService;
		LiftDetailsService m_LiftDetailsService;
		BarrierDetailsService m_BarrierDetailsService;
		EntitiesService m_EntitiesService;
		PlayersService m_PlayersService;
		CollidableService m_CollidableService;
		AimService m_AimService;
	};
}