export module Environment.Health.System;

import Service.Logs.System;
import Resolved.Vitality.Type;
import Resolved.Vitality.State;
import Tables.Object.Type;
import Tables.Object.State;
import Relations.Classifier.Type;
import Relations.Classifier.State;
import Environment.Health.Type;
import Environment.Health.State;
import std;

export namespace Environment::Health::System
{
    class HealthService
    {
    private:
        using AliveObject = Tables::Object::Type::Alive::Object;
        using DamageSectionTable = Tables::Object::Type::DamageSection::DamageSectionTable;
        using Classified = Relations::Classifier::Type::Classified;
        using ResolvedVitality = Resolved::Vitality::Type::Vitality;
        using Health = Environment::Health::Type::Health;

        using LogsService = Service::Logs::System::LogsService;
        using VitalityStore = Resolved::Vitality::State::VitalityStore;
        using ObjectTableStore = Tables::Object::State::ObjectTableStore;
        using DamageSectionsStore = Tables::Object::State::DamageSectionsStore;
        using ClassifierStore = Relations::Classifier::State::ClassifierStore;
        using HealthStore = Environment::Health::State::HealthStore;

    public:
        HealthService(LogsService& logsService, VitalityStore& vitalityStore,
            ObjectTableStore& objectStore, DamageSectionsStore& damageSectionsStore,
            ClassifierStore& classifierStore, HealthStore& healthStore) : 
            m_LogsService(logsService), m_VitalityStore(vitalityStore),
            m_ObjectStore(objectStore), m_DamageSectionsStore(damageSectionsStore),
            m_ClassifierStore(classifierStore), m_HealthStore(healthStore) {}
        ~HealthService() = default;

        auto Update() -> void;

        auto Cleanup() -> void;

    private:
        LogsService& m_LogsService;
        VitalityStore& m_VitalityStore;
        ObjectTableStore& m_ObjectStore;
        DamageSectionsStore& m_DamageSectionsStore;
        ClassifierStore& m_ClassifierStore;
        HealthStore& m_HealthStore;

        static auto HasHealth(const Classified& classified) -> bool;

        auto MakeHealth(const AliveObject& object, const ResolvedVitality& layout,
            const DamageSectionTable& damage) const -> Health;
    };
}