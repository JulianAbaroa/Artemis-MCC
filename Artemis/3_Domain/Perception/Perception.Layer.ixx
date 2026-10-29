export module Perception.Layer;

import Common.Log.System;
import Blam.MapModel.Layer;
import Blam.Runtime.Layer;
import Perception.Classifier.State;
import Perception.Classifier.System;
import Perception.ObjectGraph.State;
import Perception.ObjectGraph.System;
import Perception.Details.State;
import Perception.Details.System;
import Perception.Entities.State;
import Perception.Entities.System;
import Perception.Players.State;
import Perception.Players.System;
import Perception.Collidable.System;
import Perception.Aim.State;
import Perception.Aim.System;
import Perception.Events.State;
import Perception.Events.System;
import Perception.Match.State;
import Perception.Match.System;

export namespace Perception
{
    // Classifies the live objects and relates them in the object graph, then builds the entities, players, details, aim, events and
    // match status of the world from the tables.
    // note: Pure: the owner connects the lifecycle (D178).
    class Layer
    {
    private:
        using Log = Common::Log::System::Log;

        using ClassifierStore = Perception::Classifier::State::ClassifierStore;
        using TagClassificationsStore = Perception::Classifier::State::TagClassificationsStore;
        using ObjectGraphStore = Perception::ObjectGraph::State::ObjectGraphStore;
        using VitalityDetailsStore = Perception::Details::State::VitalityDetailsStore;
        using ZoneDetailsStore = Perception::Details::State::ZoneDetailsStore;
        using TeleporterDetailsStore = Perception::Details::State::TeleporterDetailsStore;
        using LiftDetailsStore = Perception::Details::State::LiftDetailsStore;
        using BarrierDetailsStore = Perception::Details::State::BarrierDetailsStore;
        using EntitiesStore = Perception::Entities::State::EntitiesStore;
        using CarriedHandlesStore = Perception::Entities::State::CarriedHandlesStore;
        using PlayersStore = Perception::Players::State::PlayersStore;
        using AimStore = Perception::Aim::State::AimStore;
        using EventsStore = Perception::Events::State::EventsStore;
        using MatchStore = Perception::Match::State::MatchStore;

        using ClassifierService = Perception::Classifier::System::ClassifierService;
        using ObjectGraphService = Perception::ObjectGraph::System::ObjectGraphService;
        using VitalityDetailsService = Perception::Details::System::VitalityDetailsService;
        using ZoneDetailsService = Perception::Details::System::ZoneDetailsService;
        using TeleporterDetailsService = Perception::Details::System::TeleporterDetailsService;
        using LiftDetailsService = Perception::Details::System::LiftDetailsService;
        using BarrierDetailsService = Perception::Details::System::BarrierDetailsService;
        using EntitiesService = Perception::Entities::System::EntitiesService;
        using PlayersService = Perception::Players::System::PlayersService;
        using CollidableService = Perception::Collidable::System::CollidableService;
        using AimService = Perception::Aim::System::AimService;
        using EventsService = Perception::Events::System::EventsService;
        using MatchService = Perception::Match::System::MatchService;

    public:
        Layer(Log& log, Blam::MapModel::Layer& resolved, Blam::Runtime::Layer& tables) :
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