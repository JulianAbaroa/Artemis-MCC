export module Perception.Details.System:LiftDetails;

import Common.Log.System;
import Blam.Runtime.Object.State;
import Perception.Classifier.State;
import Perception.Details.Type;
import Perception.Details.State;
import std;

export namespace Perception::Details::System
{
    // Builds the LiftDetails: the launch direction of every lift (D91).
    class LiftDetailsService
    {
    private:
        using Log = Common::Log::System::Log;

        using ObjectTableStore = Blam::Runtime::Object::State::ObjectTableStore;

        using ClassifierStore = Perception::Classifier::State::ClassifierStore;

        using LiftDetailsStore = Perception::Details::State::LiftDetailsStore;

    public:
        LiftDetailsService(Log& log, ObjectTableStore& objectStore, ClassifierStore& classifierStore,
            LiftDetailsStore& liftDetailsStore) :
            m_Log(log), m_ObjectStore(objectStore), m_ClassifierStore(classifierStore),
            m_LiftDetailsStore(liftDetailsStore) {}
        ~LiftDetailsService() = default;

        // Publishes the lift detail of every classified object with Function Lift.
        // note: Runs after the classifier of the same tick. The kind of each lift is its subtype.
        auto Update() -> void;

        // Drops the published details.
        auto Cleanup() -> void;

    private:
        Log& m_Log;
        ObjectTableStore& m_ObjectStore;
        ClassifierStore& m_ClassifierStore;
        LiftDetailsStore& m_LiftDetailsStore;
    };
}