export module Relations.Layer;

import Common.Log.System;
import Resolved.Layer;
import Tables.Layer;
import Relations.Classifier.State;
import Relations.Classifier.System;
import Relations.ObjectGraph.State;
import Relations.ObjectGraph.System;

export namespace Relations
{
    // Classifies the live objects and relates them to each other in the object graph.
    // note: Pure: the owner connects the lifecycle (D178).
    class Layer
    {
    private:
        using Log = Common::Log::System::Log;

        using ClassifierStore = Relations::Classifier::State::ClassifierStore;
        using TagClassificationsStore = Relations::Classifier::State::TagClassificationsStore;
        using ObjectGraphStore = Relations::ObjectGraph::State::ObjectGraphStore;

        using ClassifierService = Relations::Classifier::System::ClassifierService;
        using ObjectGraphService = Relations::ObjectGraph::System::ObjectGraphService;

    public:
        Layer(Log& log, Resolved::Layer& resolved, Tables::Layer& tables) :
            m_ClassifierService(log, resolved.m_DefinitionsStore, resolved.m_VitalityStore, tables.m_ObjectStore, m_ClassifierStore, m_TagClassificationsStore),
            m_ObjectGraphService(log, tables.m_ObjectStore, m_ObjectGraphStore) {}
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

        // Removes the classification and the object graph.
        auto Cleanup() -> void
        {
            m_ClassifierService.Cleanup();
            m_ObjectGraphService.Cleanup();
        }
    };
}