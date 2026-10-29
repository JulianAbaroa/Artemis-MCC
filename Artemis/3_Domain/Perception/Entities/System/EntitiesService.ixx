export module Perception.Entities.System;

import Common.Team.Type;
import Common.Log.System;
import Blam.Runtime.Object.Type;
import Blam.Runtime.Object.State;
import Blam.Runtime.Player.State;
import Perception.Classifier.Type;
import Perception.Classifier.State;
import Perception.Details.Type;
import Perception.Details.State;
import Perception.Entities.Type;
import Perception.Entities.State;
import std;

export namespace Perception::Entities::System
{
    // Builds the Entities table: one uniform row per live object (D19).
    // note: Adds to the classification the state and the capabilities that change with the object: Lifecycle, IsDynamic, IsPickable
    // and IsDestructible. The lifecycle comes from the VitalityDetails of the same tick (D87).
    // note: The team of a biped comes from its player in the player table, not from its multiplayer block (D94).
    class EntitiesService
    {
    private:
        using Team = Common::Team::Type::Team;

        using Log = Common::Log::System::Log;

        using AliveObject = Blam::Runtime::Object::Type::Alive::AliveObject;
        using ObjectTable = Blam::Runtime::Object::Type::Alive::ObjectTable;
        using ObjectTableStore = Blam::Runtime::Object::State::ObjectTableStore;

        using PlayerTableStore = Blam::Runtime::Player::State::PlayerTableStore;

        using Classified = Perception::Classifier::Type::Classified;
        using ClassifierStore = Perception::Classifier::State::ClassifierStore;

        using VitalityDetails = Perception::Details::Type::VitalityDetails;
        using VitalityDetailsStore = Perception::Details::State::VitalityDetailsStore;

        using Entity = Perception::Entities::Type::Entity;
        using Entities = Perception::Entities::Type::Entities;
        using EntitiesStore = Perception::Entities::State::EntitiesStore;
        using CarriedHandlesStore = Perception::Entities::State::CarriedHandlesStore;

    public:
        EntitiesService(Log& log, ObjectTableStore& objectStore, PlayerTableStore& playerStore, ClassifierStore& classifierStore,
            VitalityDetailsStore& vitalityDetailsStore, EntitiesStore& entitiesStore, CarriedHandlesStore& carriedHandlesStore) :
            m_Log(log), m_ObjectStore(objectStore), m_PlayerStore(playerStore), m_ClassifierStore(classifierStore),
            m_VitalityDetailsStore(vitalityDetailsStore), m_EntitiesStore(entitiesStore), m_CarriedHandlesStore(carriedHandlesStore) {}
        ~EntitiesService() = default;

        // Publishes one entity per classified object, sorted by handle.
        // note: Runs after the classifier, the player table and the vitalities of the same tick.
        auto Update() -> void;

        // Drops the published entities and the carried history.
        auto Cleanup() -> void;

    private:
        Log& m_Log;
        ObjectTableStore& m_ObjectStore;
        PlayerTableStore& m_PlayerStore;
        ClassifierStore& m_ClassifierStore;
        VitalityDetailsStore& m_VitalityDetailsStore;
        EntitiesStore& m_EntitiesStore;
        CarriedHandlesStore& m_CarriedHandlesStore;

        // Builds the entity of a classified object with its lifecycle and its dynamic capabilities.
        // param bipedTeams: Team of the player of each biped, alive or a corpse.
        auto MakeEntity(const Classified& classified, const AliveObject& object, const ObjectTable& objectTable,
            const VitalityDetails& vitalities, const std::unordered_map<std::uint32_t, Team>& bipedTeams) -> Entity;

        // Returns the last object of the parent chain, or the object itself when it is Free.
        auto RootOf(const AliveObject& object, const ObjectTable& objectTable) const -> const AliveObject&;
    };
}