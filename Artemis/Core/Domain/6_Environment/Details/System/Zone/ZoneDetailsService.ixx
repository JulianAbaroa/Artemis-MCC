export module Environment.Details.System:ZoneDetails;

import Service.Logs.System;
import Tables.Object.State;
import Relations.Classifier.State;
import Environment.Details.Type;
import Environment.Details.State;
import std;

export namespace Environment::Details::System
{
    // Builds the ZoneDetails: the zone shape of every object with HasZone (D88).
    class ZoneDetailsService
    {
    private:
        using LogsService = Service::Logs::System::LogsService;

        using ObjectTableStore = Tables::Object::State::ObjectTableStore;

        using ClassifierStore = Relations::Classifier::State::ClassifierStore;

        using ZoneDetailsStore = Environment::Details::State::ZoneDetailsStore;

    public:
        ZoneDetailsService(LogsService& logsService, ObjectTableStore& objectStore, ClassifierStore& classifierStore,
            ZoneDetailsStore& zoneDetailsStore) :
            m_LogsService(logsService), m_ObjectStore(objectStore), m_ClassifierStore(classifierStore),
            m_ZoneDetailsStore(zoneDetailsStore) {}
        ~ZoneDetailsService() = default;

        // Publishes the zone detail of every classified object with HasZone.
        // note: Runs after the classifier of the same tick.
        auto Update() -> void;

        // Drops the published details.
        auto Cleanup() -> void;

    private:
        LogsService& m_LogsService;
        ObjectTableStore& m_ObjectStore;
        ClassifierStore& m_ClassifierStore;
        ZoneDetailsStore& m_ZoneDetailsStore;
    };
}