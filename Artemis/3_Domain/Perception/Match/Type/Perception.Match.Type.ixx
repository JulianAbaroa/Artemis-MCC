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

        // Ticks the round has been played (with a time limit: limit minus ticks left), 0 when not known.
        std::uint32_t RoundClockTicks{};
        // Ticks left in the round, -1 when the round has no time limit.
        std::int32_t RoundTicksRemaining{ -1 };
        // Time limit of a round in ticks, 0 for none.
        std::uint32_t RoundTimeLimitTicks{};
        // Limit of rounds of the game variant, as stored.
        std::uint8_t RoundLimit{};
        // Score a team (a player in free for all) needs to win, as stored. 0 for none or not known.
        std::int32_t ScoreToWin{};
    };
}