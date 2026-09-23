export module Environment.Layer;

import Service.Layer;
import Platform.Layer;
import Resolved.Layer;
import Tables.Layer;
import Relations.Layer;
import Environment.Collidable.State;
import Environment.Collidable.System;
import Environment.Fixtures.State;
import Environment.Fixtures.System;
import Environment.Health.State;
import Environment.Health.System;

export namespace Environment
{
	class Layer
	{
	private:
		using CollidableStore = Environment::Collidable::State::CollidableStore;
		using FixturesStore = Environment::Fixtures::State::FixturesStore;
		using HealthStore = Environment::Health::State::HealthStore;

		using CollidableService = Environment::Collidable::System::CollidableService;
		using FixturesService = Environment::Fixtures::System::FixturesService;
		using HealthService = Environment::Health::System::HealthService;

	public:
		Layer(Service::Layer& service, Platform::Layer& platform,
			Resolved::Layer& resolved, Tables::Layer& tables,
			Relations::Layer& relations) :
			m_CollidableService(service.m_LogsService, tables.m_ObjectStore, tables.m_BoneMatricesStore, tables.m_DamageSectionsStore, relations.m_ClassifierStore, relations.m_ObjectGraphStore, resolved.m_WorldStore, m_CollidableStore),
			m_FixturesService(service.m_LogsService, tables.m_ObjectStore, relations.m_ClassifierStore, m_FixturesStore),
			m_HealthService(service.m_LogsService, resolved.m_VitalityStore, tables.m_ObjectStore, tables.m_DamageSectionsStore, relations.m_ClassifierStore, m_HealthStore)
		{
			platform.m_LifecycleService.OnCleanup([this] {
				m_CollidableService.Cleanup();
				m_FixturesService.Cleanup();
				m_HealthService.Cleanup();
			});
		}
		~Layer() = default;

		Layer(const Layer&) = delete;
		Layer& operator=(const Layer&) = delete;

		// --- State ---
		CollidableStore m_CollidableStore;
		FixturesStore m_FixturesStore;
		HealthStore m_HealthStore;

		// --- System ---
		CollidableService m_CollidableService;
		FixturesService m_FixturesService;
		HealthService m_HealthService;
	};
}