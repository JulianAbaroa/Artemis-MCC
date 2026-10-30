export module Perception.Details.System:ObjectiveDetails;

import Common.Log.System;
import Blam.Runtime.Object.State;
import Blam.Runtime.Game.State;
import Perception.Classifier.State;
import Perception.Entities.State;
import Perception.Players.State;
import Perception.Details.Type;
import Perception.Details.State;
import std;

export namespace Perception::Details::System
{
    // Builds the ObjectiveDetails: the state of every flag and the owner of every flag stand in the round (A14).
    class ObjectiveDetailsService
    {
    private:
        using Log = Common::Log::System::Log;

        using ObjectTableStore = Blam::Runtime::Object::State::ObjectTableStore;
        using GameStore = Blam::Runtime::Game::State::GameStore;

        using ClassifierStore = Perception::Classifier::State::ClassifierStore;
        using EntitiesStore = Perception::Entities::State::EntitiesStore;
        using PlayersStore = Perception::Players::State::PlayersStore;

        using ZoneDetailsStore = Perception::Details::State::ZoneDetailsStore;
        using ObjectiveDetailsStore = Perception::Details::State::ObjectiveDetailsStore;

    public:
        ObjectiveDetailsService(Log& log, ObjectTableStore& objectStore, GameStore& gameStore, ClassifierStore& classifierStore,
            EntitiesStore& entitiesStore, PlayersStore& playersStore, ZoneDetailsStore& zoneDetailsStore,
            ObjectiveDetailsStore& objectiveDetailsStore) :
            m_Log(log), m_ObjectStore(objectStore), m_GameStore(gameStore), m_ClassifierStore(classifierStore),
            m_EntitiesStore(entitiesStore), m_PlayersStore(playersStore), m_ZoneDetailsStore(zoneDetailsStore),
            m_ObjectiveDetailsStore(objectiveDetailsStore) {}
        ~ObjectiveDetailsService() = default;

        // Publishes the flags and stands of the tick.
        // note: Runs after the Players of the same tick, because the carrier and the allies are bipeds of players. The Megalo state of a
        // flag is looked up by its handle, never by element. Without a usable Megalo state a flag is Home when it stands within 1.0 of its
        // stand and Dropped anywhere else.
        auto Update() -> void;

        // Drops the published details.
        auto Cleanup() -> void;

    private:
        Log& m_Log;
        ObjectTableStore& m_ObjectStore;
        GameStore& m_GameStore;
        ClassifierStore& m_ClassifierStore;
        EntitiesStore& m_EntitiesStore;
        PlayersStore& m_PlayersStore;
        ZoneDetailsStore& m_ZoneDetailsStore;
        ObjectiveDetailsStore& m_ObjectiveDetailsStore;
    };
}