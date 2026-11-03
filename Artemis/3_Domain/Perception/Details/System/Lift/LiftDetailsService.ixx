export module Perception.Details.System:LiftDetails;

import Common.Log.System;
import Blam.MapModel.Definitions.State;
import Blam.Runtime.Game.State;
import Blam.Runtime.Object.State;
import Perception.Classifier.State;
import Perception.Details.Type;
import Perception.Details.State;
import std;

export namespace Perception::Details::System
{
    // Builds the LiftDetails: the stages of the phantom types of every lift and the measured exit of the last biped it pushed (D98, D261).
    class LiftDetailsService
    {
    private:
        using Log = Common::Log::System::Log;

        using DefinitionsStore = Blam::MapModel::Definitions::State::DefinitionsStore;
        using GameStore = Blam::Runtime::Game::State::GameStore;
        using ObjectTableStore = Blam::Runtime::Object::State::ObjectTableStore;

        using ClassifierStore = Perception::Classifier::State::ClassifierStore;

        using LiftDetailsStore = Perception::Details::State::LiftDetailsStore;
        using LiftMemoryStore = Perception::Details::State::LiftMemoryStore;

    public:
        LiftDetailsService(Log& log, DefinitionsStore& definitionsStore, ObjectTableStore& objectStore, GameStore& gameStore,
            ClassifierStore& classifierStore, LiftMemoryStore& liftMemoryStore, LiftDetailsStore& liftDetailsStore) :
            m_Log(log), m_DefinitionsStore(definitionsStore), m_ObjectStore(objectStore), m_GameStore(gameStore),
            m_ClassifierStore(classifierStore), m_LiftMemoryStore(liftMemoryStore), m_LiftDetailsStore(liftDetailsStore) {}
        ~LiftDetailsService() = default;

        // Publishes the lift detail of every classified object with Function Lift.
        // note: Runs after the classifier of the same tick. The stages come from the physics and render models of the tag, so they are
        // empty without definitions. The exit is measured: a biped that enters a phantom volume (obje flag) near a lift is assigned to it,
        // and when the flag clears the lift keeps the tick and the velocity of the biped. The engine tick is 0 without a game.
        auto Update() -> void;

        // Drops the published details and the memory.
        auto Cleanup() -> void;

    private:
        Log& m_Log;
        DefinitionsStore& m_DefinitionsStore;
        ObjectTableStore& m_ObjectStore;
        GameStore& m_GameStore;
        ClassifierStore& m_ClassifierStore;
        LiftMemoryStore& m_LiftMemoryStore;
        LiftDetailsStore& m_LiftDetailsStore;
    };
}