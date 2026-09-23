export module Egocentric.Layer;

import Service.Layer;
import Platform.Layer;
import Tables.Layer;
import Relations.Layer;
import Environment.Layer;
import Egocentric.Self.State;
import Egocentric.Self.System;
import Egocentric.Affordance.State;
import Egocentric.Affordance.System;

export namespace Egocentric
{
	class Layer
	{
	private:
		using SelfStore = Egocentric::Self::State::SelfStore;
		using AffordanceStore = Egocentric::Affordance::State::AffordanceStore;

		using SelfService = Egocentric::Self::System::SelfService;
		using AffordanceService = Egocentric::Affordance::System::AffordanceService;

	public:
		Layer(Service::Layer& service, Platform::Layer& platform,
			Tables::Layer& tables, Relations::Layer& relations,
			Environment::Layer& environment) :
			m_SelfService(service.m_LogsService, tables.m_PlayerStore, m_SelfStore),
			m_AffordanceService(service.m_LogsService, tables.m_ObjectStore, tables.m_BoneMatricesStore, tables.m_InteractionStore, m_SelfStore, relations.m_ClassifierStore, relations.m_ObjectGraphStore, relations.m_PlayerGraphStore, m_AffordanceStore, environment.m_CollidableService)
		{
			platform.m_LifecycleService.OnCleanup([this] {
				m_SelfService.Cleanup();
				m_AffordanceService.Cleanup();
			});
		}
		~Layer() = default;

		Layer(const Layer&) = delete;
		Layer& operator=(const Layer&) = delete;

		// --- State ---
		SelfStore m_SelfStore;
		AffordanceStore m_AffordanceStore;

		// --- System ---
		SelfService m_SelfService;
		AffordanceService m_AffordanceService;
	};
}