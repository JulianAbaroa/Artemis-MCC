export module Environment.Details.System:BarrierDetails;

import Service.Logs.System;
import Tables.Object.State;
import Relations.Classifier.State;
import Environment.Details.Type;
import Environment.Details.State;
import std;

export namespace Environment::Details::System
{
    // Builds the BarrierDetails: the pass direction of every barrier (D91).
    class BarrierDetailsService
    {
    private:
        using LogsService = Service::Logs::System::LogsService;

        using ObjectTableStore = Tables::Object::State::ObjectTableStore;

        using ClassifierStore = Relations::Classifier::State::ClassifierStore;

        using BarrierDetailsStore = Environment::Details::State::BarrierDetailsStore;

    public:
        BarrierDetailsService(LogsService& logsService, ObjectTableStore& objectStore, ClassifierStore& classifierStore,
            BarrierDetailsStore& barrierDetailsStore) :
            m_LogsService(logsService), m_ObjectStore(objectStore), m_ClassifierStore(classifierStore),
            m_BarrierDetailsStore(barrierDetailsStore) {}
        ~BarrierDetailsService() = default;

        // Publishes the barrier detail of every classified object with Function Barrier.
        // note: Runs after the classifier of the same tick. The kind of each barrier is its subtype.
        auto Update() -> void;

        // Drops the published details.
        auto Cleanup() -> void;

    private:
        LogsService& m_LogsService;
        ObjectTableStore& m_ObjectStore;
        ClassifierStore& m_ClassifierStore;
        BarrierDetailsStore& m_BarrierDetailsStore;
    };
}