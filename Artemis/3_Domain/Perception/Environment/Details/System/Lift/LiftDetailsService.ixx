export module Environment.Details.System:LiftDetails;

import Common.Log.System;
import Tables.Object.State;
import Relations.Classifier.State;
import Environment.Details.Type;
import Environment.Details.State;
import std;

export namespace Environment::Details::System
{
    // Builds the LiftDetails: the launch direction of every lift (D91).
    class LiftDetailsService
    {
    private:
        using Log = Common::Log::System::Log;

        using ObjectTableStore = Tables::Object::State::ObjectTableStore;

        using ClassifierStore = Relations::Classifier::State::ClassifierStore;

        using LiftDetailsStore = Environment::Details::State::LiftDetailsStore;

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