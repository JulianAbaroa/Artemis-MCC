export module Bridge.Codec.Tick.System;

import Blam.MapModel.Definitions.Type;
import Blam.MapModel.Definitions.State;
import Blam.Runtime.Interaction.Type;
import Blam.Runtime.Object.State;
import Blam.Runtime.Player.State;
import Blam.Runtime.Interaction.State;
import Perception.Classifier.State;
import Perception.ObjectGraph.State;
import Perception.Details.State;
import Perception.Entities.State;
import Perception.Players.State;
import Perception.Aim.State;
import Perception.Events.State;
import Perception.Match.State;
import Views.Self.State;
import Views.Affordance.State;
import Views.Raycast.State;
import Bridge.Codec.Tick.Type;
import Bridge.Codec.Tick.State;
import std;

export namespace Bridge::Codec::Tick::System
{
    class TickService
    {
    private:
        using ObjectTableStore = Blam::Runtime::Object::State::ObjectTableStore;
        using PlayerTableStore = Blam::Runtime::Player::State::PlayerTableStore;
        using InteractionStore = Blam::Runtime::Interaction::State::InteractionStore;
        using ClassifierStore = Perception::Classifier::State::ClassifierStore;
        using ObjectGraphStore = Perception::ObjectGraph::State::ObjectGraphStore;
        using EntitiesStore = Perception::Entities::State::EntitiesStore;
        using PlayersStore = Perception::Players::State::PlayersStore;
        using VitalityDetailsStore = Perception::Details::State::VitalityDetailsStore;
        using ZoneDetailsStore = Perception::Details::State::ZoneDetailsStore;
        using TeleporterDetailsStore = Perception::Details::State::TeleporterDetailsStore;
        using LiftDetailsStore = Perception::Details::State::LiftDetailsStore;
        using BarrierDetailsStore = Perception::Details::State::BarrierDetailsStore;
        using ObjectiveDetailsStore = Perception::Details::State::ObjectiveDetailsStore;
        using ProjectileDetailsStore = Perception::Details::State::ProjectileDetailsStore;
        using WeaponDetailsStore = Perception::Details::State::WeaponDetailsStore;
        using AimStore = Perception::Aim::State::AimStore;
        using EventsStore = Perception::Events::State::EventsStore;
        using MatchStore = Perception::Match::State::MatchStore;
        using SelfStore = Views::Self::State::SelfStore;
        using AffordanceStore = Views::Affordance::State::AffordanceStore;
        using RaycastStore = Views::Raycast::State::RaycastStore;
        using DefinitionsStore = Blam::MapModel::Definitions::State::DefinitionsStore;
        using TickStore = Bridge::Codec::Tick::State::TickStore;
        using Sbsp = Blam::MapModel::Definitions::Type::Sbsp::Sbsp;
        using Scnr = Blam::MapModel::Definitions::Type::Scnr::Scnr;
        using Sddt = Blam::MapModel::Definitions::Type::Sddt::Sddt;

    public:
        TickService(ObjectTableStore& objectStore, PlayerTableStore& playerStore,
            InteractionStore& interactionStore, ClassifierStore& classifierStore,
            ObjectGraphStore& objectGraphStore,
            EntitiesStore& entitiesStore, PlayersStore& playersStore, VitalityDetailsStore& vitalityDetailsStore, ZoneDetailsStore& zoneDetailsStore,
            TeleporterDetailsStore& teleporterDetailsStore, LiftDetailsStore& liftDetailsStore, BarrierDetailsStore& barrierDetailsStore,
            ObjectiveDetailsStore& objectiveDetailsStore, ProjectileDetailsStore& projectileDetailsStore, WeaponDetailsStore& weaponDetailsStore,
            AimStore& aimStore, EventsStore& eventsStore, MatchStore& matchStore, SelfStore& selfStore,
            AffordanceStore& affordanceStore, RaycastStore& raycastStore,
            DefinitionsStore& definitionsStore, TickStore& tickStore) :
            m_ObjectStore(objectStore), m_PlayerStore(playerStore),
            m_InteractionStore(interactionStore), m_ClassifierStore(classifierStore),
            m_ObjectGraphStore(objectGraphStore),
            m_EntitiesStore(entitiesStore), m_PlayersStore(playersStore), m_VitalityDetailsStore(vitalityDetailsStore), m_ZoneDetailsStore(zoneDetailsStore),
            m_TeleporterDetailsStore(teleporterDetailsStore), m_LiftDetailsStore(liftDetailsStore), m_BarrierDetailsStore(barrierDetailsStore),
            m_ObjectiveDetailsStore(objectiveDetailsStore), m_ProjectileDetailsStore(projectileDetailsStore), m_WeaponDetailsStore(weaponDetailsStore),
            m_AimStore(aimStore), m_EventsStore(eventsStore), m_MatchStore(matchStore), m_SelfStore(selfStore),
            m_AffordanceStore(affordanceStore), m_RaycastStore(raycastStore),
            m_DefinitionsStore(definitionsStore), m_TickStore(tickStore) {}
        ~TickService() = default;

        auto Assemble(std::uint64_t generation) -> void;

    private:
        ObjectTableStore& m_ObjectStore;
        PlayerTableStore& m_PlayerStore;
        InteractionStore& m_InteractionStore;
        ClassifierStore& m_ClassifierStore;
        ObjectGraphStore& m_ObjectGraphStore;
        EntitiesStore& m_EntitiesStore;
        PlayersStore& m_PlayersStore;
        VitalityDetailsStore& m_VitalityDetailsStore;
        ZoneDetailsStore& m_ZoneDetailsStore;
        TeleporterDetailsStore& m_TeleporterDetailsStore;
        LiftDetailsStore& m_LiftDetailsStore;
        BarrierDetailsStore& m_BarrierDetailsStore;
        ObjectiveDetailsStore& m_ObjectiveDetailsStore;
        ProjectileDetailsStore& m_ProjectileDetailsStore;
        WeaponDetailsStore& m_WeaponDetailsStore;
        AimStore& m_AimStore;
        EventsStore& m_EventsStore;
        MatchStore& m_MatchStore;
        SelfStore& m_SelfStore;
        AffordanceStore& m_AffordanceStore;
        RaycastStore& m_RaycastStore;
        DefinitionsStore& m_DefinitionsStore;
        TickStore& m_TickStore;
        std::shared_ptr<const std::unordered_map<std::string, Sbsp>> m_Map{};
        std::shared_ptr<const std::unordered_map<std::string, Scnr>> m_Limits{};
        std::shared_ptr<const std::unordered_map<std::string, Sddt>> m_Designs{};
    };
}