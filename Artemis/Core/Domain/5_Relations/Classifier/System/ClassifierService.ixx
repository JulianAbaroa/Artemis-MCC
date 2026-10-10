export module Relations.Classifier.System;

import Service.Logs.System;
import Tables.Object.Type;
import Tables.Object.State;
import Relations.Classifier.Type;
import Relations.Classifier.State;
import std;

export namespace Relations::Classifier::System
{
    class ClassifierService
    {
    private:
        using AliveObject = Tables::Object::Type::Alive::Object;
        using ObjectTable = std::unordered_map<std::uint32_t, AliveObject>;
        using Role = Relations::Classifier::Type::Role;

        using LogsService = Service::Logs::System::LogsService;
        using ObjectTableStore = Tables::Object::State::ObjectTableStore;
        using ClassifierStore = Relations::Classifier::State::ClassifierStore;

    public:
        ClassifierService(LogsService& logsService, ObjectTableStore& objectStore,
            ClassifierStore& classifierStore) : m_LogsService(logsService),
            m_ObjectStore(objectStore), m_ClassifierStore(classifierStore) {}
        ~ClassifierService() = default;

        auto UpdateClassification() -> void;

        auto Cleanup() -> void;

    private:
        LogsService& m_LogsService;
        ObjectTableStore& m_ObjectStore;
        ClassifierStore& m_ClassifierStore;

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