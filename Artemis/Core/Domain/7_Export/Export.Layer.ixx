export module Export.Layer;

import Service.Layer;
import Platform.Layer;
import Tables.Layer;
import Relations.Layer;
import Environment.Layer;
import Egocentric.Layer;
import Export.Tick.State;
import Export.Tick.System;
import Export.Tick.Hook;

export namespace Export
{
	class Layer
	{
	private:
		using TickStore = Export::Tick::State::TickStore;
		using TickService = Export::Tick::System::TickService;
		using SimulationTicksDetour = Export::Tick::Hook::SimulationTicksDetour;

	public:
		Layer(Service::Layer& service, Platform::Layer& platform,
			Tables::Layer& tables, Relations::Layer& relations,
			Environment::Layer& environment, Egocentric::Layer& egocentric) :
			m_TickService(tables.m_ObjectStore, tables.m_PlayerStore, tables.m_InteractionStore, relations.m_ClassifierStore, relations.m_ObjectGraphStore, relations.m_PlayerGraphStore, environment.m_CollidableStore, environment.m_FixturesStore, environment.m_HealthStore, egocentric.m_SelfStore, egocentric.m_AffordanceStore, egocentric.m_RaycastStore, m_TickStore),
			m_SimulationTicksDetour(service.m_LogsService, service.m_TelemetryStore, platform.m_AOBService, platform.m_LifecycleStore)
		{
			auto& lifecycle = platform.m_LifecycleService;

			lifecycle.OnEngineInitialized([this] {
				m_SimulationTicksDetour.Install();
			});

			lifecycle.OnUnhook([this] {
				m_SimulationTicksDetour.Uninstall();
			});

			lifecycle.OnCleanup([this] {
				m_TickStore.Cleanup();
			});
		}
		~Layer() = default;

		Layer(const Layer&) = delete;
		Layer& operator=(const Layer&) = delete;

		// --- State ---
		TickStore m_TickStore;

		// --- System ---
		TickService m_TickService;

		// --- Hook ---
		SimulationTicksDetour m_SimulationTicksDetour;
	};
}