export module Environment.Match.Type;

import Common.Semantic.Type;
import std;

namespace
{
    using Common::Semantic::Type::MatchPhase;
}

export namespace Environment::Match::Type
{
    // Score of one player.
    struct MatchScore
    {
        std::uint32_t PlayerHandle{};
        std::int16_t Score{};
    };

    // State of the match in one tick (D103).
    struct MatchStatus
    {
        // Ticks since the start of the match (60 per second).
        std::uint32_t GameTick{};
        MatchPhase Phase{ MatchPhase::Unknown };
        // One per player of the player table, by table index.
        std::vector<MatchScore> Scores{};
    };
}