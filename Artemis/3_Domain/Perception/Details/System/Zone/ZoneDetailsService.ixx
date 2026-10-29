export module Perception.Details.System:ZoneDetails;

import Common.Log.System;
import Blam.Runtime.Object.State;
import Perception.Classifier.State;
import Perception.Details.Type;
import Perception.Details.State;
import std;

export namespace Perception::Details::System
{
    // Builds the ZoneDetails: the zone shape of every object with HasZone (D88).
    class ZoneDetailsService
    {
    private:
        using Log = Common::Log::System::Log;

        using ObjectTableStore = Blam::Runtime::Object::State::ObjectTableStore;

        using ClassifierStore = Perception::Classifier::State::ClassifierStore;

        using ZoneDetailsStore = Perception::Details::State::ZoneDetailsStore;

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