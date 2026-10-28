export module Environment.Details.System:ZoneDetails;

import Common.Log.System;
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
        using Log = Common::Log::System::Log;

        using ObjectTableStore = Tables::Object::State::ObjectTableStore;

        using ClassifierStore = Relations::Classifier::State::ClassifierStore;

        using ZoneDetailsStore = Environment::Details::State::ZoneDetailsStore;

    public:
        ZoneDetailsService(Log& log, ObjectTableStore& objectStore, ClassifierStore& classifierStore,
            ZoneDetailsStore& zoneDetailsStore) :
            m_Log(log), m_ObjectStore(objectStore), m_ClassifierStore(classifierStore),
            m_ZoneDetailsStore(zoneDetailsStore) {}
        ~ZoneDetailsService() = default;

        // Publishes the zone detail of every classified object with HasZone.
        // note: Runs after the classifier of the same tick.
        auto Update() -> void;

        // Drops the published details.
        auto Cleanup() -> void;

    private:
        Log& m_Log;
        ObjectTableStore& m_ObjectStore;
        ClassifierStore& m_ClassifierStore;
        ZoneDetailsStore& m_ZoneDetailsStore;
    };
}