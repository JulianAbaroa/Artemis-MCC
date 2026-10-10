export module Relations.Layer;

import Service.Layer;
import Platform.Layer;
import Tables.Layer;
import Relations.Classifier.State;
import Relations.Classifier.System;
import Relations.ObjectGraph.State;
import Relations.ObjectGraph.System;
import Relations.PlayerGraph.State;
import Relations.PlayerGraph.System;

export namespace Relations
{
	class Layer
	{
	private:
		using ClassifierStore = Relations::Classifier::State::ClassifierStore;
		using ObjectGraphStore = Relations::ObjectGraph::State::ObjectGraphStore;
		using PlayerGraphStore = Relations::PlayerGraph::State::PlayerGraphStore;

		using ClassifierService = Relations::Classifier::System::ClassifierService;
		using ObjectGraphService = Relations::ObjectGraph::System::ObjectGraphService;
		using PlayerGraphService = Relations::PlayerGraph::System::PlayerGraphService;

	public:
		Layer(Service::Layer& service, Platform::Layer& platform, Tables::Layer& tables) :
			m_ClassifierService(service.m_LogsService, tables.m_ObjectStore, m_ClassifierStore),
			m_ObjectGraphService(service.m_LogsService, tables.m_ObjectStore, m_ObjectGraphStore),
			m_PlayerGraphService(service.m_LogsService, tables.m_ObjectStore, tables.m_PlayerStore, m_ClassifierStore, m_ObjectGraphStore, m_PlayerGraphStore)
		{
			platform.m_LifecycleService.OnCleanup([this] {
				m_ClassifierService.Cleanup();
				m_ObjectGraphService.Cleanup();
				m_PlayerGraphService.Cleanup();
			});
		}
		~Layer() = default;

		Layer(const Layer&) = delete;
		Layer& operator=(const Layer&) = delete;

		// --- State ---
		ClassifierStore m_ClassifierStore;
		ObjectGraphStore m_ObjectGraphStore;
		PlayerGraphStore m_PlayerGraphStore;

		// --- System ---
		ClassifierService m_ClassifierService;
		ObjectGraphService m_ObjectGraphService;
		PlayerGraphService m_PlayerGraphService;
	};
}