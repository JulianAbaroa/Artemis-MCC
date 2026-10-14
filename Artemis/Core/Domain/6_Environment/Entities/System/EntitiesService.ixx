export module Environment.Entities.System;

import Service.Logs.System;
import Resolved.Vitality.Type;
import Resolved.Vitality.State;
import Tables.Object.Type;
import Tables.Object.State;
import Relations.Classifier.Type;
import Relations.Classifier.State;
import Environment.Entities.Type;
import Environment.Entities.State;
import std;

export namespace Environment::Entities::System
{
    // Builds the Entities table: one uniform row per live object (Ignore/Semantizacion/05, section 9).
    // note: Adds to the classification the state and the capabilities that change with the object: Lifecycle, IsDynamic, IsPickable
    // and IsDestructible.
    class EntitiesService
    {
    private:
        using LogsService = Service::Logs::System::LogsService;

        using Vitality = Resolved::Vitality::Type::Vitality::Vitality;
        using VitalityStore = Resolved::Vitality::State::VitalityStore;

        using AliveObject = Tables::Object::Type::Alive::Object;
        using ObjectTable = std::unordered_map<std::uint32_t, AliveObject>;
        using ObjectTableStore = Tables::Object::State::ObjectTableStore;
        using DamageSectionsStore = Tables::Object::State::DamageSectionsStore;

        using Classified = Relations::Classifier::Type::Classified;
        using ClassifierStore = Relations::Classifier::State::ClassifierStore;

        using Entity = Environment::Entities::Type::Entity;
        using Entities = Environment::Entities::Type::Entities;
        using EntitiesStore = Environment::Entities::State::EntitiesStore;
        using CarriedHandlesStore = Environment::Entities::State::CarriedHandlesStore;

    public:
        EntitiesService(LogsService& logsService, VitalityStore& vitalityStore, ObjectTableStore& objectStore,
            DamageSectionsStore& damageSectionsStore, ClassifierStore& classifierStore, EntitiesStore& entitiesStore,
            CarriedHandlesStore& carriedHandlesStore) :
            m_LogsService(logsService), m_VitalityStore(vitalityStore), m_ObjectStore(objectStore),
            m_DamageSectionsStore(damageSectionsStore), m_ClassifierStore(classifierStore), m_EntitiesStore(entitiesStore),
            m_CarriedHandlesStore(carriedHandlesStore) {}
        ~EntitiesService() = default;

        // Publishes one entity per classified object, sorted by handle.
        // note: Runs after the classifier of the same tick.
        auto Update() -> void;

        // Drops the published entities and the carried history.
        auto Cleanup() -> void;

    private:
        LogsService& m_LogsService;
        VitalityStore& m_VitalityStore;
        ObjectTableStore& m_ObjectStore;
        DamageSectionsStore& m_DamageSectionsStore;
        ClassifierStore& m_ClassifierStore;
        EntitiesStore& m_EntitiesStore;
        CarriedHandlesStore& m_CarriedHandlesStore;

        // Builds the entity of a classified object with its lifecycle and its dynamic capabilities.
        auto MakeEntity(const Classified& classified, const AliveObject& object, const ObjectTable& objectTable) -> Entity;

        // Returns the last object of the parent chain, or the object itself when it is Free.
        auto RootOf(const AliveObject& object, const ObjectTable& objectTable) const -> const AliveObject&;

        // Returns the vitality of the first damage section, from 0 to 1. The object is dead when it reaches 0 (D85).
        // return: Empty when the tag has no vitality layout or the datum has no damage sections.
        auto FindVitality(const AliveObject& object) const -> std::optional<float>;
    };
}