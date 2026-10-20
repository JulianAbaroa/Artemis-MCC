export module Environment.Details.System:VitalityDetails;

import Service.Logs.System;
import Resolved.Vitality.Type;
import Resolved.Vitality.State;
import Tables.Object.Type;
import Tables.Object.State;
import Relations.Classifier.State;
import Environment.Details.Type;
import Environment.Details.State;
import std;

export namespace Environment::Details::System
{
    // Builds the VitalityDetails: the vitality of every damage section of the objects with HasVitality (D84, D87).
    // note: Runs before Entities, which takes the lifecycle from it.
    class VitalityDetailsService
    {
    private:
        using LogsService = Service::Logs::System::LogsService;

        using Vitality = Resolved::Vitality::Type::Vitality::Vitality;
        using VitalityStore = Resolved::Vitality::State::VitalityStore;

        using AliveObject = Tables::Object::Type::Alive::AliveObject;
        using ObjectTableStore = Tables::Object::State::ObjectTableStore;
        using DamageSectionsStore = Tables::Object::State::DamageSectionsStore;

        using ClassifierStore = Relations::Classifier::State::ClassifierStore;

        using VitalityDetail = Environment::Details::Type::VitalityDetail;
        using VitalityDetailsStore = Environment::Details::State::VitalityDetailsStore;

    public:
        VitalityDetailsService(LogsService& logsService, VitalityStore& vitalityStore, ObjectTableStore& objectStore,
            DamageSectionsStore& damageSectionsStore, ClassifierStore& classifierStore, VitalityDetailsStore& vitalityDetailsStore) :
            m_LogsService(logsService), m_VitalityStore(vitalityStore), m_ObjectStore(objectStore),
            m_DamageSectionsStore(damageSectionsStore), m_ClassifierStore(classifierStore), m_VitalityDetailsStore(vitalityDetailsStore) {}
        ~VitalityDetailsService() = default;

        // Publishes the vitality detail of every classified object with HasVitality.
        // note: Runs after the classifier of the same tick.
        auto Update() -> void;

        // Drops the published details.
        auto Cleanup() -> void;

    private:
        LogsService& m_LogsService;
        VitalityStore& m_VitalityStore;
        ObjectTableStore& m_ObjectStore;
        DamageSectionsStore& m_DamageSectionsStore;
        ClassifierStore& m_ClassifierStore;
        VitalityDetailsStore& m_VitalityDetailsStore;

        // Reads the vitality of every section of the layout of the object.
        // return: Empty when the tag has no vitality layout or the datum has no damage sections.
        auto MakeVitality(const AliveObject& object) const -> std::optional<VitalityDetail>;
    };
}