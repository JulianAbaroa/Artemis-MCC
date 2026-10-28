export module Environment.Layer;

import Common.Log.System;
import Resolved.Layer;
import Tables.Layer;
import Relations.Classifier.State;
import Relations.Classifier.System;
import Relations.ObjectGraph.State;
import Relations.ObjectGraph.System;
import Environment.Details.State;
import Environment.Details.System;
import Environment.Entities.State;
import Environment.Entities.System;
import Environment.Players.State;
import Environment.Players.System;
import Environment.Collidable.System;
import Environment.Aim.State;
import Environment.Aim.System;
import Environment.Events.State;
import Environment.Events.System;
import Environment.Match.State;
import Environment.Match.System;

export namespace Environment
{
    // Classifies the live objects and relates them in the object graph, then builds the entities, players, details, aim, events and
    // match status of the world from the tables.
    // note: Pure: the owner connects the lifecycle (D178).
    class Layer
    {
    private:
        using Log = Common::Log::System::Log;

        using ClassifierStore = Relations::Classifier::State::ClassifierStore;
        using TagClassificationsStore = Relations::Classifier::State::TagClassificationsStore;
        using ObjectGraphStore = Relations::ObjectGraph::State::ObjectGraphStore;
        using VitalityDetailsStore = Environment::Details::State::VitalityDetailsStore;
        using ZoneDetailsStore = Environment::Details::State::ZoneDetailsStore;
        using TeleporterDetailsStore = Environment::Details::State::TeleporterDetailsStore;
        using LiftDetailsStore = Environment::Details::State::LiftDetailsStore;
        using BarrierDetailsStore = Environment::Details::State::BarrierDetailsStore;
        using EntitiesStore = Environment::Entities::State::EntitiesStore;
        using CarriedHandlesStore = Environment::Entities::State::CarriedHandlesStore;
        using PlayersStore = Environment::Players::State::PlayersStore;
        using AimStore = Environment::Aim::State::AimStore;
        using EventsStore = Environment::Events::State::EventsStore;
        using MatchStore = Environment::Match::State::MatchStore;

        using ClassifierService = Relations::Classifier::System::ClassifierService;
        using ObjectGraphService = Relations::ObjectGraph::System::ObjectGraphService;
        using VitalityDetailsService = Environment::Details::System::VitalityDetailsService;
        using ZoneDetailsService = Environment::Details::System::ZoneDetailsService;
        using TeleporterDetailsService = Environment::Details::System::TeleporterDetailsService;
        using LiftDetailsService = Environment::Details::System::LiftDetailsService;
        using BarrierDetailsService = Environment::Details::System::BarrierDetailsService;
        using EntitiesService = Environment::Entities::System::EntitiesService;
        using PlayersService = Environment::Players::System::PlayersService;
        using CollidableService = Environment::Collidable::System::CollidableService;
        using AimService = Environment::Aim::System::AimService;
        using EventsService = Environment::Events::System::EventsService;
        using MatchService = Environment::Match::System::MatchService;

    public:
        Layer(Log& log, Resolved::Layer& resolved, Tables::Layer& tables) :
            m_ClassifierService(log, resolved.m_DefinitionsStore, resolved.m_VitalityStore, tables.m_ObjectStore, m_ClassifierStore, m_TagClassificationsStore),
            m_ObjectGraphService(log, tables.m_ObjectStore, m_ObjectGraphStore),
            m_VitalityDetailsService(log, resolved.m_VitalityStore, tables.m_ObjectStore, tables.m_DamageSectionsStore, m_ClassifierStore, m_VitalityDetailsStore),
            m_ZoneDetailsService(log, tables.m_ObjectStore, m_ClassifierStore, m_ZoneDetailsStore),
            m_TeleporterDetailsService(log, tables.m_ObjectStore, m_ClassifierStore, m_TeleporterDetailsStore),
            m_LiftDetailsService(log, tables.m_ObjectStore, m_ClassifierStore, m_LiftDetailsStore),
            m_BarrierDetailsService(log, tables.m_ObjectStore, m_ClassifierStore, m_BarrierDetailsStore),
            m_EntitiesService(log, tables.m_ObjectStore, tables.m_PlayerStore, m_ClassifierStore, m_VitalityDetailsStore, m_EntitiesStore, m_CarriedHandlesStore),
            m_PlayersService(log, tables.m_ObjectStore, tables.m_PlayerStore, m_VitalityDetailsStore, m_EntitiesStore, m_PlayersStore),
            m_CollidableService(log, resolved.m_DefinitionsStore, tables.m_ObjectStore, tables.m_BoneMatricesStore, tables.m_DamageSectionsStore, m_ClassifierStore, resolved.m_WorldStore),
            m_AimService(log, resolved.m_VitalityStore, tables.m_ObjectStore, tables.m_BoneMatricesStore, m_AimStore),
            m_EventsService(tables.m_GameStore, m_EventsStore),
            m_MatchService(tables.m_PlayerStore, tables.m_GameStore, m_MatchStore) {}
        ~Layer() = default;

        Layer(const Layer&) = delete;
        auto operator=(const Layer&) -> Layer& = delete;

        // --- State ---
        ClassifierStore m_ClassifierStore{};
        TagClassificationsStore m_TagClassificationsStore{};
        ObjectGraphStore m_ObjectGraphStore{};
        VitalityDetailsStore m_VitalityDetailsStore{};
        ZoneDetailsStore m_ZoneDetailsStore{};
        TeleporterDetailsStore m_TeleporterDetailsStore{};
        LiftDetailsStore m_LiftDetailsStore{};
        BarrierDetailsStore m_BarrierDetailsStore{};
        EntitiesStore m_EntitiesStore{};
        CarriedHandlesStore m_CarriedHandlesStore{};
        PlayersStore m_PlayersStore{};
        AimStore m_AimStore{};
        EventsStore m_EventsStore{};
        MatchStore m_MatchStore{};

        // --- System ---
        ClassifierService m_ClassifierService;
        ObjectGraphService m_ObjectGraphService;
        VitalityDetailsService m_VitalityDetailsService;
        ZoneDetailsService m_ZoneDetailsService;
        TeleporterDetailsService m_TeleporterDetailsService;
        LiftDetailsService m_LiftDetailsService;
        BarrierDetailsService m_BarrierDetailsService;
        EntitiesService m_EntitiesService;
        PlayersService m_PlayersService;
        CollidableService m_CollidableService;
        AimService m_AimService;
        EventsService m_EventsService;
        MatchService m_MatchService;

        // Removes the classification, the object graph, the details, entities, players, aim, events and match status.
        auto Cleanup() -> void
        {
            m_ClassifierService.Cleanup();
            m_ObjectGraphService.Cleanup();
            m_VitalityDetailsService.Cleanup();
            m_ZoneDetailsService.Cleanup();
            m_TeleporterDetailsService.Cleanup();
            m_LiftDetailsService.Cleanup();
            m_BarrierDetailsService.Cleanup();
            m_EntitiesService.Cleanup();
            m_PlayersService.Cleanup();
            m_CollidableService.Cleanup();
            m_AimService.Cleanup();
            m_EventsService.Cleanup();
            m_MatchService.Cleanup();
        }
    };
}