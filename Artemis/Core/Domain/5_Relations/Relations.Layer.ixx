export module Relations.Layer;

import Service.Layer;
import Platform.Layer;
import Resolved.Layer;
import Tables.Layer;
import Relations.Classifier.State;
import Relations.Classifier.System;
import Relations.ObjectGraph.State;
import Relations.ObjectGraph.System;

export namespace Relations
{
    class Layer
    {
    private:
        using ClassifierStore = Relations::Classifier::State::ClassifierStore;
        using TagClassificationsStore = Relations::Classifier::State::TagClassificationsStore;
        using ObjectGraphStore = Relations::ObjectGraph::State::ObjectGraphStore;

        using ClassifierService = Relations::Classifier::System::ClassifierService;
        using ObjectGraphService = Relations::ObjectGraph::System::ObjectGraphService;

    public:
        Layer(Service::Layer& service, Platform::Layer& platform, Resolved::Layer& resolved, Tables::Layer& tables) :
            m_ClassifierService(service.m_Log, resolved.m_DefinitionsStore, resolved.m_VitalityStore, tables.m_ObjectStore, m_ClassifierStore, m_TagClassificationsStore),
            m_ObjectGraphService(service.m_Log, tables.m_ObjectStore, m_ObjectGraphStore)
        {
            platform.m_LifecycleService.OnCleanup([this] {
                m_ClassifierService.Cleanup();
                m_ObjectGraphService.Cleanup();
            });
        }
        ~Layer() = default;

        Layer(const Layer&) = delete;
        auto operator=(const Layer&) -> Layer& = delete;

        // --- State ---
        ClassifierStore m_ClassifierStore{};
        TagClassificationsStore m_TagClassificationsStore{};
        ObjectGraphStore m_ObjectGraphStore{};

        // --- System ---
        ClassifierService m_ClassifierService;
        ObjectGraphService m_ObjectGraphService;
    };
}