export module Perception.Match.Type;

import Common.Engine.Type;
import Common.Semantic.Type;
import std;

namespace
{
    using Common::Semantic::Type::MatchPhase;
    using Common::Semantic::Type::RoundState;

    using Common::Engine::Type::k_MaxTeams;
}

export namespace Perception::Match::Type
{
    // Score of one player.
    struct MatchScore
    {
        std::uint32_t PlayerHandle{};
        std::int16_t Score{};
    };

    // Score of one team: in the current round and added up over the match.
    struct MatchTeamScore
    {
        std::int16_t Round{};
        std::int16_t Total{};
    };

    // State of the match in one tick (D103).
    struct MatchStatus
    {
        // Ticks since the start of the match (60 per second).
        std::uint32_t GameTick{};
        MatchPhase Phase{ MatchPhase::Unknown };
        // One per player of the player table, by table index.
        std::vector<MatchScore> Scores{};

        // Round of the match, from 0, and its state.
        std::int16_t RoundIndex{};
        ::RoundState RoundState{ ::RoundState::Unknown };
        // Score of each team, indexed by team.
        std::array<MatchTeamScore, k_MaxTeams> TeamScores{};
    };
}