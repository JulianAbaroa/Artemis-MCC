export module Export.Tick.System;

import Resolved.Definitions.Type;
import Resolved.Definitions.State;
import Tables.Interaction.Type;
import Tables.Object.State;
import Tables.Player.State;
import Tables.Interaction.State;
import Relations.Classifier.State;
import Relations.ObjectGraph.State;
import Environment.Details.State;
import Environment.Entities.State;
import Environment.Players.State;
import Environment.Aim.State;
import Egocentric.Self.State;
import Egocentric.Affordance.State;
import Egocentric.Raycast.State;
import Export.Tick.Type;
import Export.Tick.State;
import std;

export namespace Export::Tick::System
{
    class TickService
    {
    private:
        using ObjectTableStore = Tables::Object::State::ObjectTableStore;
        using PlayerTableStore = Tables::Player::State::PlayerTableStore;
        using InteractionStore = Tables::Interaction::State::InteractionStore;
        using ClassifierStore = Relations::Classifier::State::ClassifierStore;
        using ObjectGraphStore = Relations::ObjectGraph::State::ObjectGraphStore;
        using EntitiesStore = Environment::Entities::State::EntitiesStore;
        using PlayersStore = Environment::Players::State::PlayersStore;
        using VitalityDetailsStore = Environment::Details::State::VitalityDetailsStore;
        using ZoneDetailsStore = Environment::Details::State::ZoneDetailsStore;
        using TeleporterDetailsStore = Environment::Details::State::TeleporterDetailsStore;
        using LiftDetailsStore = Environment::Details::State::LiftDetailsStore;
        using BarrierDetailsStore = Environment::Details::State::BarrierDetailsStore;
        using AimStore = Environment::Aim::State::AimStore;
        using SelfStore = Egocentric::Self::State::SelfStore;
        using AffordanceStore = Egocentric::Affordance::State::AffordanceStore;
        using RaycastStore = Egocentric::Raycast::State::RaycastStore;
        using DefinitionsStore = Resolved::Definitions::State::DefinitionsStore;
        using TickStore = Export::Tick::State::TickStore;
        using Sbsp = Resolved::Definitions::Type::Sbsp::Sbsp;
        using Scnr = Resolved::Definitions::Type::Scnr::Scnr;
        using Sddt = Resolved::Definitions::Type::Sddt::Sddt;

    public:
        TickService(ObjectTableStore& objectStore, PlayerTableStore& playerStore,
            InteractionStore& interactionStore, ClassifierStore& classifierStore,
            ObjectGraphStore& objectGraphStore,
            EntitiesStore& entitiesStore, PlayersStore& playersStore, VitalityDetailsStore& vitalityDetailsStore, ZoneDetailsStore& zoneDetailsStore,
            TeleporterDetailsStore& teleporterDetailsStore, LiftDetailsStore& liftDetailsStore, BarrierDetailsStore& barrierDetailsStore,
            AimStore& aimStore, SelfStore& selfStore,
            AffordanceStore& affordanceStore, RaycastStore& raycastStore,
            DefinitionsStore& definitionsStore, TickStore& tickStore) :
            m_ObjectStore(objectStore), m_PlayerStore(playerStore),
            m_InteractionStore(interactionStore), m_ClassifierStore(classifierStore),
            m_ObjectGraphStore(objectGraphStore),
            m_EntitiesStore(entitiesStore), m_PlayersStore(playersStore), m_VitalityDetailsStore(vitalityDetailsStore), m_ZoneDetailsStore(zoneDetailsStore),
            m_TeleporterDetailsStore(teleporterDetailsStore), m_LiftDetailsStore(liftDetailsStore), m_BarrierDetailsStore(barrierDetailsStore),
            m_AimStore(aimStore), m_SelfStore(selfStore),
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
        AimStore& m_AimStore;
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