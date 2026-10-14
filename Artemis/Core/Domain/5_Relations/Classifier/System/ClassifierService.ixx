export module Relations.Classifier.System;

import Service.Logs.System;
import Resolved.Definitions.State;
import Resolved.Vitality.State;
import Tables.Object.Type;
import Tables.Object.State;
import Relations.Classifier.Type;
import Relations.Classifier.State;
import std;

export namespace Relations::Classifier::System
{
    // Classifies every live object: Kind, Function, Subtype, Group, Binding and the static capabilities (Ignore/Semantizacion/05).
    // note: The function of a tag comes from the catalog, then the multiplayer object kind of the tag, then the default of the kind.
    // It depends on the tag only, so it is cached per tag for the whole map.
    class ClassifierService
    {
    private:
        using LogsService = Service::Logs::System::LogsService;

        using DefinitionsStore = Resolved::Definitions::State::DefinitionsStore;
        using VitalityStore = Resolved::Vitality::State::VitalityStore;

        using AliveObject = Tables::Object::Type::Alive::Object;
        using ObjectTable = std::unordered_map<std::uint32_t, AliveObject>;
        using ObjectTableStore = Tables::Object::State::ObjectTableStore;

        using Role = Relations::Classifier::Type::Role;
        using TagClassification = Relations::Classifier::Type::TagClassification;
        using ClassifierStore = Relations::Classifier::State::ClassifierStore;

    public:
        ClassifierService(LogsService& logsService, DefinitionsStore& definitionsStore, VitalityStore& vitalityStore,
            ObjectTableStore& objectStore, ClassifierStore& classifierStore) :
            m_LogsService(logsService), m_DefinitionsStore(definitionsStore), m_VitalityStore(vitalityStore),
            m_ObjectStore(objectStore), m_ClassifierStore(classifierStore) {}
        ~ClassifierService() = default;

        // Publishes the classification of every live object, Unknown ones included (D28).
        auto UpdateClassification() -> void;

        // Drops the published classification. The tag cache is dropped on the next update.
        // note: Called from the cleanup of the lifecycle, so it only marks the cache.
        auto Cleanup() -> void;

    private:
        LogsService& m_LogsService;
        DefinitionsStore& m_DefinitionsStore;
        VitalityStore& m_VitalityStore;
        ObjectTableStore& m_ObjectStore;
        ClassifierStore& m_ClassifierStore;

        // Classification of each tag of the map, by group and tag name.
        // note: Used from the update thread only.
        std::unordered_map<std::string, TagClassification> m_TagClassifications{};
        std::atomic<bool> m_IsCacheStale{ false };

        // Returns the classification of the tag of the object, from the cache when the map definitions are ready.
        // note: Logs a warning the first time a tag ends Unknown.
        auto FindTagClassification(const AliveObject& object) -> TagClassification;

        // Classifies a tag: catalog, then multiplayer object kind, then kind default. Subtypes follow the same order.
        auto ClassifyTag(const AliveObject& object) const -> TagClassification;

        auto ClassifyNode(std::uint32_t handle, const AliveObject& node,
            const std::unordered_map<std::uint32_t, AliveObject>& nodes) const -> Role;

        auto ClassifyVehicle(const AliveObject& object) const -> Role;
        auto ClassifyWeapon(const AliveObject& object) const -> Role;
        auto ClassifyEquipment(const AliveObject& object) const -> Role;
        auto ClassifyScenery(const AliveObject& object) const -> Role;
        auto ClassifyBloc(const AliveObject& object) const -> Role;
        auto ClassifyDeviceControl(const AliveObject& object) const -> Role;
    };
}