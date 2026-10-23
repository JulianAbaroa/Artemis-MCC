export module Environment.Details.System:TeleporterDetails;

import Common.Log.System;
import Tables.Object.State;
import Relations.Classifier.State;
import Environment.Details.Type;
import Environment.Details.State;
import std;

export namespace Environment::Details::System
{
    // Builds the TeleporterDetails: channel, passes and destinations of every teleporter (D90).
    class TeleporterDetailsService
    {
    private:
        using Log = Common::Log::System::Log;

        using ObjectTableStore = Tables::Object::State::ObjectTableStore;

        using ClassifierStore = Relations::Classifier::State::ClassifierStore;

        using TeleporterDetailsStore = Environment::Details::State::TeleporterDetailsStore;

    public:
        TeleporterDetailsService(Log& log, ObjectTableStore& objectStore, ClassifierStore& classifierStore,
            TeleporterDetailsStore& teleporterDetailsStore) :
            m_Log(log), m_ObjectStore(objectStore), m_ClassifierStore(classifierStore),
            m_TeleporterDetailsStore(teleporterDetailsStore) {}
        ~TeleporterDetailsService() = default;

        // Publishes the teleporter detail of every classified object with Function Teleporter.
        // note: Runs after the classifier of the same tick. The kind of each teleporter is its subtype.
        auto Update() -> void;

        // Drops the published details.
        auto Cleanup() -> void;

    private:
        Log& m_Log;
        ObjectTableStore& m_ObjectStore;
        ClassifierStore& m_ClassifierStore;
        TeleporterDetailsStore& m_TeleporterDetailsStore;
    };
}