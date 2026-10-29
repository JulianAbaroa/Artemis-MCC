export module Perception.Classifier.System;

import Common.Log.System;
import Blam.MapModel.Definitions.State;
import Blam.MapModel.Vitality.State;
import Blam.Runtime.Object.Type;
import Blam.Runtime.Object.State;
import Perception.Classifier.Type;
import Perception.Classifier.State;
import std;

export namespace Perception::Classifier::System
{
    // Classifies every live object: Kind, Function, Subtype, Group, Binding and the static capabilities (D19).
    // note: The function of a tag comes from the catalog, then the multiplayer object kind of the tag, then the default of the kind.
    // It depends on the tag only, so it is cached per tag for the whole map.
    class ClassifierService
    {
    private:
        using Log = Common::Log::System::Log;

        using DefinitionsStore = Blam::MapModel::Definitions::State::DefinitionsStore;
        using VitalityStore = Blam::MapModel::Vitality::State::VitalityStore;

        using AliveObject = Blam::Runtime::Object::Type::Alive::AliveObject;
        using ObjectTable = Blam::Runtime::Object::Type::Alive::ObjectTable;
        using ObjectTableStore = Blam::Runtime::Object::State::ObjectTableStore;

        using TagClassification = Perception::Classifier::Type::TagClassification;
        using ClassifierStore = Perception::Classifier::State::ClassifierStore;
        using TagClassificationsStore = Perception::Classifier::State::TagClassificationsStore;

    public:
        ClassifierService(Log& log, DefinitionsStore& definitionsStore, VitalityStore& vitalityStore,
            ObjectTableStore& objectStore, ClassifierStore& classifierStore, TagClassificationsStore& tagClassificationsStore) :
            m_Log(log), m_DefinitionsStore(definitionsStore), m_VitalityStore(vitalityStore),
            m_ObjectStore(objectStore), m_ClassifierStore(classifierStore), m_TagClassificationsStore(tagClassificationsStore) {}
        ~ClassifierService() = default;

        // Publishes the classification of every live object, Unknown ones included (D28).
        auto UpdateClassification() -> void;

        // Drops the published classification and the classifications of the tags.
        auto Cleanup() -> void;

    private:
        Log& m_Log;
        DefinitionsStore& m_DefinitionsStore;
        VitalityStore& m_VitalityStore;
        ObjectTableStore& m_ObjectStore;
        ClassifierStore& m_ClassifierStore;
        TagClassificationsStore& m_TagClassificationsStore;

        // Checks whether the definitions and the vitality of the map are built, so tag classifications can be kept.
        auto IsDefinitionsReady() const -> bool;

        // Logs once per map how many tags each rule classified and the functions the kind default gave.
        // note: Runs on the first update with the definitions ready, so it covers the objects the map starts with.
        auto LogSummary() -> void;

        // Returns the classification of the tag of the object, from the store once the map definitions are ready.
        // note: Logs a warning the first time a tag ends Unknown, or when the catalog and the multiplayer kind disagree on its function.
        auto FindTagClassification(const AliveObject& object) -> TagClassification;

        // Classifies a tag: catalog, then multiplayer object kind, then kind default. Subtypes follow the same order.
        auto ClassifyTag(const AliveObject& object) const -> TagClassification;
    };
}