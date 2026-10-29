export module Perception.Details.Type:Objective;

import Common.Engine.Type;
import Common.Semantic.Type;
import Common.Team.Type;
import std;

namespace
{
    using Common::Semantic::Type::FlagState;
    using Common::Team::Type::Team;

    using Common::Engine::Type::k_NoHandle;
}

export namespace Perception::Details::Type
{
    // Flag of a capture game: who owns it in this round, where it is and what is left of its reset timer.
    // note: Keyed by Handle in ObjectiveDetails. The engine recreates the flag on reset, return and capture, so the handle changes and
    // the BaseTeam is what identifies it across ticks.
    struct ObjectiveDetail
    {
        std::uint32_t Handle{};

        // Team of the placement of the flag (map variant). It never changes.
        ::Team BaseTeam{ ::Team::Neutral };

        // Team that plays the side of the flag in this round, from the team mapping of the match. Neutral when the side is not in use.
        ::Team OwnerTeam{ ::Team::Neutral };

        ::FlagState State{ ::FlagState::Home };

        // Player and biped that carry it, or 0xFFFFFFFF. A carrier without a player (a bot not in the table) leaves the player empty.
        std::uint32_t CarrierPlayer{ k_NoHandle };
        std::uint32_t CarrierBiped{ k_NoHandle };

        // Stand with the same base team, or 0xFFFFFFFF.
        std::uint32_t HomeStand{ k_NoHandle };

        // Ticks left to reset a dropped flag, rounded up. 0 when it is not dropped or the engine has no timer for it.
        std::uint32_t ResetTicks{};

        // Whether the reset timer of a dropped flag is counting down.
        bool IsResetRunning{};

        // Live bipeds of players of the OwnerTeam inside the zone of the flag.
        std::uint8_t AlliesInZone{};
    };

    // Flag stand of a capture game.
    struct StandDetail
    {
        std::uint32_t Handle{};
        ::Team BaseTeam{ ::Team::Neutral };
        ::Team OwnerTeam{ ::Team::Neutral };
    };

    // Flags and stands of the tick, by handle.
    struct ObjectiveDetails
    {
        std::unordered_map<std::uint32_t, ObjectiveDetail> Flags{};
        std::unordered_map<std::uint32_t, StandDetail> Stands{};
    };
}