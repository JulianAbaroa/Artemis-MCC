export module Environment.Entities.System;

import Common.Team.Type;
import Service.Logs.System;
import Tables.Object.Type;
import Tables.Object.State;
import Tables.Player.State;
import Relations.Classifier.Type;
import Relations.Classifier.State;
import Environment.Details.Type;
import Environment.Details.State;
import Environment.Entities.Type;
import Environment.Entities.State;
import std;

export namespace Environment::Entities::System
{
    // Builds the Entities table: one uniform row per live object (D19).
    // note: Adds to the classification the state and the capabilities that change with the object: Lifecycle, IsDynamic, IsPickable
    // and IsDestructible. The lifecycle comes from the VitalityDetails of the same tick (D87).
    // note: The team of a biped comes from its player in the player table, not from its multiplayer block (D94).
    class EntitiesService
    {
    private:
        using Team = Common::Team::Type::Team;

        using LogsService = Service::Logs::System::LogsService;

        using AliveObject = Tables::Object::Type::Alive::AliveObject;
        using ObjectTable = Tables::Object::Type::Alive::ObjectTable;
        using ObjectTableStore = Tables::Object::State::ObjectTableStore;

        using PlayerTableStore = Tables::Player::State::PlayerTableStore;

        using Classified = Relations::Classifier::Type::Classified;
        using ClassifierStore = Relations::Classifier::State::ClassifierStore;

        using VitalityDetails = Environment::Details::Type::VitalityDetails;
        using VitalityDetailsStore = Environment::Details::State::VitalityDetailsStore;

        using Entity = Environment::Entities::Type::Entity;
        using Entities = Environment::Entities::Type::Entities;
        using EntitiesStore = Environment::Entities::State::EntitiesStore;
        using CarriedHandlesStore = Environment::Entities::State::CarriedHandlesStore;

    public:
        EntitiesService(LogsService& logsService, ObjectTableStore& objectStore, PlayerTableStore& playerStore, ClassifierStore& classifierStore,
            VitalityDetailsStore& vitalityDetailsStore, EntitiesStore& entitiesStore, CarriedHandlesStore& carriedHandlesStore) :
            m_LogsService(logsService), m_ObjectStore(objectStore), m_PlayerStore(playerStore), m_ClassifierStore(classifierStore),
            m_VitalityDetailsStore(vitalityDetailsStore), m_EntitiesStore(entitiesStore), m_CarriedHandlesStore(carriedHandlesStore) {}
        ~EntitiesService() = default;

        // Publishes one entity per classified object, sorted by handle.
        // note: Runs after the classifier, the player table and the vitalities of the same tick.
        auto Update() -> void;

        // Drops the published entities and the carried history.
        auto Cleanup() -> void;

    private:
        LogsService& m_LogsService;
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