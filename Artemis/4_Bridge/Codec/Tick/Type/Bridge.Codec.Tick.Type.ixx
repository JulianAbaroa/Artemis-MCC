export module Bridge.Codec.Tick.Type;

import Blam.MapModel.Definitions.Type;
import Blam.Runtime.Object.Type;
import Blam.Runtime.Player.Type;
import Blam.Runtime.Interaction.Type;
import Perception.Classifier.Type;
import Perception.ObjectGraph.Type;
import Perception.Details.Type;
import Perception.Entities.Type;
import Perception.Players.Type;
import Perception.Aim.Type;
import Perception.Events.Type;
import Perception.Match.Type;
import Views.Affordance.Type;
import Views.Self.Type;
import Views.Raycast.Type;
import std;

namespace
{
    using Blam::Runtime::Object::Type::Alive::AliveObject;
    using Blam::Runtime::Object::Type::Alive::ObjectTable;
    using Blam::Runtime::Player::Type::Alive::AlivePlayer;
    using Blam::Runtime::Player::Type::Alive::PlayerTable;
    using Blam::Runtime::Interaction::Type::Alive::AliveInteraction;

    using Perception::Classifier::Type::Classified;
    using Perception::Classifier::Type::Classifieds;
    using Perception::ObjectGraph::Type::ObjectNode;
    using Perception::ObjectGraph::Type::ObjectNodes;

    using Perception::Entities::Type::Entity;
    using Perception::Entities::Type::Entities;
    using Perception::Players::Type::Body;
    using Perception::Players::Type::Player;
    using Perception::Players::Type::Players;
    using Perception::Details::Type::VitalityDetail;
    using Perception::Details::Type::VitalityDetails;
    using Perception::Details::Type::ZoneDetail;
    using Perception::Details::Type::ZoneDetails;
    using Perception::Details::Type::TeleporterDetail;
    using Perception::Details::Type::TeleporterDetails;
    using Perception::Details::Type::LiftDetail;
    using Perception::Details::Type::LiftDetails;
    using Perception::Details::Type::ExplosiveDetail;
    using Perception::Details::Type::ExplosiveDetails;
    using Perception::Details::Type::BarrierDetail;
    using Perception::Details::Type::BarrierDetails;
    using Perception::Details::Type::ObjectiveDetails;
    using Perception::Details::Type::ProjectileDetails;
    using Perception::Details::Type::WeaponDetails;
    using Perception::Aim::Type::Aim;
    using Perception::Aim::Type::Aims;
    using Perception::Events::Type::EventLog;
    using Perception::Match::Type::MatchStatus;

    using Views::Affordance::Type::Affordance;
    using Views::Affordance::Type::Affordances;
    using Views::Self::Type::Self;
    using Views::Raycast::Type::Raycasts;
}

export namespace Bridge::Codec::Tick::Type
{
    struct Tick
    {
        std::uint64_t Generation = 0;
        // MapId of the scnr of the loaded map, engine id. 0 while no map or the scnr is not frozen.
        std::int32_t MapId = 0;

        std::shared_ptr<const std::unordered_map<std::string, Blam::MapModel::Definitions::Type::Sbsp::Sbsp>> Map{};
        std::shared_ptr<const std::unordered_map<std::string, Blam::MapModel::Definitions::Type::Scnr::Scnr>> Limits{};
        std::shared_ptr<const std::unordered_map<std::string, Blam::MapModel::Definitions::Type::Sddt::Sddt>> Designs{};

        // --- Layer 3: Tables ---
        std::shared_ptr<const ObjectTable> ObjectTable{};
        std::shared_ptr<const PlayerTable> PlayerTable{};
        // Interaction of the agent (local player Views::Self::Type::k_SelfLocalIndex).
        std::shared_ptr<const AliveInteraction> Interaction{};

        // --- Layer 4: Relations ---
        std::shared_ptr<const Classifieds> Classifieds{};
        std::shared_ptr<const ObjectNodes> ObjectGraph{};

        // --- Layer 5: Environment ---
        std::shared_ptr<const Entities> Entities{};
        std::shared_ptr<const Players> Players{};
        std::shared_ptr<const VitalityDetails> VitalityDetails{};
        std::shared_ptr<const ZoneDetails> ZoneDetails{};
        std::shared_ptr<const TeleporterDetails> TeleporterDetails{};
        std::shared_ptr<const LiftDetails> LiftDetails{};
        std::shared_ptr<const BarrierDetails> BarrierDetails{};
        std::shared_ptr<const ExplosiveDetails> ExplosiveDetails{};
        std::shared_ptr<const ObjectiveDetails> ObjectiveDetails{};
        std::shared_ptr<const ProjectileDetails> ProjectileDetails{};
        std::shared_ptr<const WeaponDetails> WeaponDetails{};
        std::shared_ptr<const Aims> Aims{};
        // Last events of the match. Readers keep their own cursor by sequence (D104).
        std::shared_ptr<const EventLog> Events{};
        std::shared_ptr<const MatchStatus> Match{};

        // --- Layer 6: Egocentric ---
        std::shared_ptr<const Self> Self{};
        std::shared_ptr<const Affordances> Affordances{};
        std::shared_ptr<const Raycasts> Raycasts{};
    };
}