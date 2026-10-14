export module Environment.Entities.System;

import Service.Logs.System;
import Tables.Object.Type;
import Tables.Object.State;
import Relations.Classifier.Type;
import Relations.Classifier.State;
import Environment.Details.Type;
import Environment.Details.State;
import Environment.Entities.Type;
import Environment.Entities.State;
import std;

export namespace Environment::Entities::System
{
    // Builds the Entities table: one uniform row per live object (Ignore/Semantizacion/05, section 9).
    // note: Adds to the classification the state and the capabilities that change with the object: Lifecycle, IsDynamic, IsPickable
    // and IsDestructible. The lifecycle comes from the VitalityDetails of the same tick (D87).
    class EntitiesService
    {
    private:
        using LogsService = Service::Logs::System::LogsService;

        using AliveObject = Tables::Object::Type::Alive::Object;
        using ObjectTable = std::unordered_map<std::uint32_t, AliveObject>;
        using ObjectTableStore = Tables::Object::State::ObjectTableStore;

        using Classified = Relations::Classifier::Type::Classified;
        using ClassifierStore = Relations::Classifier::State::ClassifierStore;

        using VitalityDetails = Environment::Details::Type::VitalityDetails;
        using VitalityDetailsStore = Environment::Details::State::VitalityDetailsStore;

        using Entity = Environment::Entities::Type::Entity;
        using Entities = Environment::Entities::Type::Entities;
        using EntitiesStore = Environment::Entities::State::EntitiesStore;
        using CarriedHandlesStore = Environment::Entities::State::CarriedHandlesStore;

    public:
        EntitiesService(LogsService& logsService, ObjectTableStore& objectStore, ClassifierStore& classifierStore,
            VitalityDetailsStore& vitalityDetailsStore, EntitiesStore& entitiesStore, CarriedHandlesStore& carriedHandlesStore) :
            m_LogsService(logsService), m_ObjectStore(objectStore), m_ClassifierStore(classifierStore),
            m_VitalityDetailsStore(vitalityDetailsStore), m_EntitiesStore(entitiesStore), m_CarriedHandlesStore(carriedHandlesStore) {}
        ~EntitiesService() = default;

        // Publishes one entity per classified object, sorted by handle.
        // note: Runs after the classifier and the vitalities of the same tick.
        auto Update() -> void;

        // Drops the published entities and the carried history.
        auto Cleanup() -> void;

    private:
        LogsService& m_LogsService;
        ObjectTableStore& m_ObjectStore;
        ClassifierStore& m_ClassifierStore;
        VitalityDetailsStore& m_VitalityDetailsStore;
        EntitiesStore& m_EntitiesStore;
        CarriedHandlesStore& m_CarriedHandlesStore;

        // Builds the entity of a classified object with its lifecycle and its dynamic capabilities.
        auto MakeEntity(const Classified& classified, const AliveObject& object, const ObjectTable& objectTable,
            const VitalityDetails& vitalities) -> Entity;

        // Returns the last object of the parent chain, or the object itself when it is Free.
        auto RootOf(const AliveObject& object, const ObjectTable& objectTable) const -> const AliveObject&;
    };
}