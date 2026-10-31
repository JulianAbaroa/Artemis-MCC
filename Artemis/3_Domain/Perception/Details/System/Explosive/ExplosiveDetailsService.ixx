export module Perception.Details.System:ExplosiveDetails;

import Common.Log.System;
import Blam.MapModel.Definitions.State;
import Blam.Runtime.Object.State;
import Perception.Classifier.State;
import Perception.Details.Type;
import Perception.Details.State;
import std;

export namespace Perception::Details::System
{
    // Builds the ExplosiveDetails: the radius and damage of the explosion of every explosive, from its tags (D259).
    class ExplosiveDetailsService
    {
    private:
        using Log = Common::Log::System::Log;

        using DefinitionsStore = Blam::MapModel::Definitions::State::DefinitionsStore;

        using ObjectTableStore = Blam::Runtime::Object::State::ObjectTableStore;

        using ClassifierStore = Perception::Classifier::State::ClassifierStore;

        using ExplosiveDetailsStore = Perception::Details::State::ExplosiveDetailsStore;

    public:
        ExplosiveDetailsService(Log& log, DefinitionsStore& definitionsStore, ObjectTableStore& objectStore,
            ClassifierStore& classifierStore, ExplosiveDetailsStore& explosiveDetailsStore) :
            m_Log(log), m_DefinitionsStore(definitionsStore), m_ObjectStore(objectStore), m_ClassifierStore(classifierStore),
            m_ExplosiveDetailsStore(explosiveDetailsStore) {}
        ~ExplosiveDetailsService() = default;

        // Publishes the explosive detail of every classified object with Function Explosive.
        // note: Runs after the classifier of the same tick. Without definitions (no map) every explosion is 0.
        auto Update() -> void;

        // Drops the published details.
        auto Cleanup() -> void;

    private:
        Log& m_Log;
        DefinitionsStore& m_DefinitionsStore;
        ObjectTableStore& m_ObjectStore;
        ClassifierStore& m_ClassifierStore;
        ExplosiveDetailsStore& m_ExplosiveDetailsStore;
    };
}