export module Export.Tick.Type;

import Resolved.Definitions.Type;
import Tables.Object.Type;
import Tables.Player.Type;
import Tables.Interaction.Type;
import Relations.Classifier.Type;
import Relations.ObjectGraph.Type;
import Environment.Details.Type;
import Environment.Entities.Type;
import Environment.Players.Type;
import Environment.Aim.Type;
import Environment.Events.Type;
import Environment.Match.Type;
import Egocentric.Affordance.Type;
import Egocentric.Self.Type;
import Egocentric.Raycast.Type;
import std;

namespace
{
    using Tables::Object::Type::Alive::AliveObject;
    using Tables::Object::Type::Alive::ObjectTable;
    using Tables::Player::Type::Alive::AlivePlayer;
    using Tables::Player::Type::Alive::PlayerTable;
    using Tables::Interaction::Type::Alive::AliveInteraction;

    using Relations::Classifier::Type::Classified;
    using Relations::Classifier::Type::Classifieds;
    using Relations::ObjectGraph::Type::ObjectNode;
    using Relations::ObjectGraph::Type::ObjectNodes;

    using Environment::Entities::Type::Entity;
    using Environment::Entities::Type::Entities;
    using Environment::Players::Type::Body;
    using Environment::Players::Type::Player;
    using Environment::Players::Type::Players;
    using Environment::Details::Type::VitalityDetail;
    using Environment::Details::Type::VitalityDetails;
    using Environment::Details::Type::ZoneDetail;
    using Environment::Details::Type::ZoneDetails;
    using Environment::Details::Type::TeleporterDetail;
    using Environment::Details::Type::TeleporterDetails;
    using Environment::Details::Type::LiftDetail;
    using Environment::Details::Type::LiftDetails;
    using Environment::Details::Type::BarrierDetail;
    using Environment::Details::Type::BarrierDetails;
    using Environment::Aim::Type::Aim;
    using Environment::Aim::Type::Aims;
    using Environment::Events::Type::EventLog;
    using Environment::Match::Type::MatchStatus;

    using Egocentric::Affordance::Type::Affordance;
    using Egocentric::Affordance::Type::Affordances;
    using Egocentric::Self::Type::Self;
    using Egocentric::Raycast::Type::Raycasts;
}

export namespace Export::Tick::Type
{
    // Engine function that runs the logic ticks of one frame (FUN_180057434).
    // note: remainingTime is the frame time left after the ticks. The engine lowers it if it stops before ticksToAdvance.
    using SimulationTicksFunction = auto(__fastcall*)(int ticksToAdvance, float* remainingTime) -> void;

    // Engine function that runs one logic tick (FUN_1800575AC). SimulationTicks calls it once per tick.
    using ExecuteGameFrameFunction = auto(__fastcall*)() -> void;

    // Engine function that ends the round (FUN_18007b3ec, D144).
    // param winner: Team or player index, or k_EndRoundNoWinner to let the scores decide.
    // param isGameEnd: Ends the match too. The engine also ends it when the round limit is reached.
    // param reason: Why the round ends, read by the engine for the winner and the end of the match.
    using EndRoundFunction = auto(__fastcall*)(std::int32_t winner, bool isGameEnd, std::int32_t reason) -> void;

    // Arguments of the end_round action of Megalo: no fixed winner, reason 0 (D144).
    inline constexpr std::int32_t k_EndRoundNoWinner{ -1 };
    inline constexpr std::int32_t k_EndRoundScriptReason{ 0 };

    struct Tick
    {
        std::uint64_t Generation = 0;

        std::shared_ptr<const std::unordered_map<std::string, Resolved::Definitions::Type::Sbsp::Sbsp>> Map{};
        std::shared_ptr<const std::unordered_map<std::string, Resolved::Definitions::Type::Scnr::Scnr>> Limits{};
        std::shared_ptr<const std::unordered_map<std::string, Resolved::Definitions::Type::Sddt::Sddt>> Designs{};

        // --- Layer 3: Tables ---
        std::shared_ptr<const ObjectTable> ObjectTable{};
        std::shared_ptr<const PlayerTable> PlayerTable{};
        // Interaction of the agent (local player Egocentric::Self::Type::k_SelfLocalIndex).
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