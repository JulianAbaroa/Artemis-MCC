export module Perception.Details.System:BarrierDetails;

import Common.Log.System;
import Blam.Runtime.Object.State;
import Perception.Classifier.State;
import Perception.Details.Type;
import Perception.Details.State;
import std;

export namespace Perception::Details::System
{
    // Builds the BarrierDetails: the pass direction of every barrier (D91).
    class BarrierDetailsService
    {
    private:
        using Log = Common::Log::System::Log;

        using ObjectTableStore = Blam::Runtime::Object::State::ObjectTableStore;

        using ClassifierStore = Perception::Classifier::State::ClassifierStore;

        using BarrierDetailsStore = Perception::Details::State::BarrierDetailsStore;

    public:
        BarrierDetailsService(Log& log, ObjectTableStore& objectStore, ClassifierStore& classifierStore,
            BarrierDetailsStore& barrierDetailsStore) :
            m_Log(log), m_ObjectStore(objectStore), m_ClassifierStore(classifierStore),
            m_BarrierDetailsStore(barrierDetailsStore) {}
        ~BarrierDetailsService() = default;

        // Publishes the barrier detail of every classified object with Function Barrier.
        // note: Runs after the classifier of the same tick. The kind of each barrier is its subtype.
        auto Update() -> void;

        // Drops the published details.
        auto Cleanup() -> void;

    private:
        Log& m_Log;
        ObjectTableStore& m_ObjectStore;
        ClassifierStore& m_ClassifierStore;
        BarrierDetailsStore& m_BarrierDetailsStore;
    };
}