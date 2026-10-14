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
import Environment.Collidable.State;
import Environment.Collidable.System;
import Environment.Fixtures.State;
import Environment.Fixtures.System;
import Environment.Aim.State;
import Environment.Aim.System;

export namespace Environment
{
	class Layer
	{
	private:
		using VitalityDetailsStore = Environment::Details::State::VitalityDetailsStore;
		using EntitiesStore = Environment::Entities::State::EntitiesStore;
		using CarriedHandlesStore = Environment::Entities::State::CarriedHandlesStore;
		using CollidableStore = Environment::Collidable::State::CollidableStore;
		using FixturesStore = Environment::Fixtures::State::FixturesStore;
		using AimStore = Environment::Aim::State::AimStore;

		using VitalityDetailsService = Environment::Details::System::VitalityDetailsService;
		using EntitiesService = Environment::Entities::System::EntitiesService;
		using CollidableService = Environment::Collidable::System::CollidableService;
		using FixturesService = Environment::Fixtures::System::FixturesService;
		using AimService = Environment::Aim::System::AimService;

	public:
		Layer(Service::Layer& service, Platform::Layer& platform, Resolved::Layer& resolved,
			Tables::Layer& tables, Relations::Layer& relations) :
			m_VitalityDetailsService(service.m_LogsService, resolved.m_VitalityStore, tables.m_ObjectStore, tables.m_DamageSectionsStore, relations.m_ClassifierStore, m_VitalityDetailsStore),
			m_EntitiesService(service.m_LogsService, tables.m_ObjectStore, relations.m_ClassifierStore, m_VitalityDetailsStore, m_EntitiesStore, m_CarriedHandlesStore),
			m_CollidableService(service.m_LogsService, resolved.m_DefinitionsStore, tables.m_ObjectStore, tables.m_BoneMatricesStore, tables.m_DamageSectionsStore, relations.m_ClassifierStore, resolved.m_WorldStore, m_CollidableStore),
			m_FixturesService(service.m_LogsService, tables.m_ObjectStore, relations.m_ClassifierStore, m_FixturesStore),
			m_AimService(service.m_LogsService, resolved.m_VitalityStore, tables.m_ObjectStore, tables.m_BoneMatricesStore, m_AimStore)
		{
			platform.m_LifecycleService.OnCleanup([this] {
				m_VitalityDetailsService.Cleanup();
				m_EntitiesService.Cleanup();
				m_CollidableService.Cleanup();
				m_FixturesService.Cleanup();
				m_AimService.Cleanup();
			});
		}
		~Layer() = default;

		Layer(const Layer&) = delete;
		Layer& operator=(const Layer&) = delete;

		// --- State ---
		VitalityDetailsStore m_VitalityDetailsStore;
		EntitiesStore m_EntitiesStore;
		CarriedHandlesStore m_CarriedHandlesStore;
		CollidableStore m_CollidableStore;
		FixturesStore m_FixturesStore;
		AimStore m_AimStore;

		// --- System ---
		VitalityDetailsService m_VitalityDetailsService;
		EntitiesService m_EntitiesService;
		CollidableService m_CollidableService;
		FixturesService m_FixturesService;
		AimService m_AimService;
	};
}