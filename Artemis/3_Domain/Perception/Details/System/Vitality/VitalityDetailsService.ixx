export module Perception.Details.System:VitalityDetails;

import Common.Log.System;
import Blam.MapModel.Vitality.Type;
import Blam.MapModel.Vitality.State;
import Blam.Runtime.Object.Type;
import Blam.Runtime.Object.State;
import Perception.Classifier.State;
import Perception.Details.Type;
import Perception.Details.State;
import std;

export namespace Perception::Details::System
{
    // Builds the VitalityDetails: the vitality of every damage section of the objects with HasVitality (D84, D87).
    // note: Runs before Entities, which takes the lifecycle from it.
    class VitalityDetailsService
    {
    private:
        using Log = Common::Log::System::Log;

        using Vitality = Blam::MapModel::Vitality::Type::Vitality::Vitality;
        using VitalityStore = Blam::MapModel::Vitality::State::VitalityStore;

        using AliveObject = Blam::Runtime::Object::Type::Alive::AliveObject;
        using ObjectTableStore = Blam::Runtime::Object::State::ObjectTableStore;
        using DamageSectionsStore = Blam::Runtime::Object::State::DamageSectionsStore;

        using ClassifierStore = Perception::Classifier::State::ClassifierStore;

        using VitalityDetail = Perception::Details::Type::VitalityDetail;
        using VitalityDetailsStore = Perception::Details::State::VitalityDetailsStore;

    public:
        VitalityDetailsService(Log& log, VitalityStore& vitalityStore, ObjectTableStore& objectStore,
            DamageSectionsStore& damageSectionsStore, ClassifierStore& classifierStore, VitalityDetailsStore& vitalityDetailsStore) :
            m_Log(log), m_VitalityStore(vitalityStore), m_ObjectStore(objectStore),
            m_DamageSectionsStore(damageSectionsStore), m_ClassifierStore(classifierStore), m_VitalityDetailsStore(vitalityDetailsStore) {}
        ~VitalityDetailsService() = default;

        // Publishes the vitality detail of every classified object with HasVitality.
        // note: Runs after the classifier of the same tick.
        auto Update() -> void;

        // Drops the published details.
        auto Cleanup() -> void;

    private:
        Log& m_Log;
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