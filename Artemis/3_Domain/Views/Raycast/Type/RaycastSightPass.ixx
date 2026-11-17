export module Views.Raycast.Type:SightPass;

import Common.Engine.Type;
import Common.Math.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;

    using Common::Engine::Type::k_MaxPlayers;
}

// What the sight pass reads of each player and what it answers (D310, F4.3).

export namespace Views::Raycast::Type
{
    // One player as the sight pass sees it.
    struct SightPlayer
    {
        // Alive and with a valid eye: only these players see and are seen.
        bool HasHead{};

        // Where the player looks from (the derived eye) and the direction of its aim.
        Vec3 Head{};
        Vec3 Aim{};

        // Whether the player is an agent in HumanLike mode: the only kind that looks through a cone.
        bool IsViewer{};
    };

    // The players of a tick, by row of Players.
    struct SightPlayers : std::array<SightPlayer, k_MaxPlayers>
    {
    };

    // Who sees whom: [a][b] is true when the player of row a sees the player of row b (b in its cone and the line between the heads free).
    struct SightSeen : std::array<std::array<bool, k_MaxPlayers>, k_MaxPlayers>
    {
    };

    // What one sight pass did, for the tests and the bench.
    struct SightCounters
    {
        // Unordered pairs that passed the range and the cone of at least one viewer.
        std::size_t Candidates{ 0 };

        // Rays cast: one per candidate pair, none for two heads at the same point.
        std::size_t Rays{ 0 };
    };
}