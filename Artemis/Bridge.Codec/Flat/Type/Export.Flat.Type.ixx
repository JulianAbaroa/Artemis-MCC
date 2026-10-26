export module Export.Flat.Type;

import Common.Engine.Type;
import Export.Tick.Type;
import std;

namespace
{
    using Common::Engine::Type::k_MaxPlayers;
}

export namespace Export::Flat::Type
{
    // Action tick and applied decisions read while the game thread waits for the read of a tick, so they match the state of the
    // tables copied in that tick.
    struct BridgeClock
    {
        // Last action tick the engine applied before this state.
        std::uint64_t ActionTick{ 0 };

        // DecisionTick of the bridge action applied to each player (by player table index), 0 = none.
        std::array<std::uint64_t, k_MaxPlayers> AppliedDecisions{};

        // Whether this state is a decision tick of a connected client (a tick goes out).
        bool IsDecision{ false };
    };

    // A Tick of a decision tick waiting to be flattened into a BridgeTick, with the clock read with it (D135).
    struct FlatRequest
    {
        std::shared_ptr<const Export::Tick::Type::Tick> Snapshot{};
        BridgeClock Clock{};
    };
}