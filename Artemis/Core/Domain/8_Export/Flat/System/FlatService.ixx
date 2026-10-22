export module Export.Flat.System;

import Platform.Bridge.Type;
import Export.Tick.Type;
import std;

export namespace Export::Flat::System
{
    // Flattens a Tick into a BridgeTick: Players, Entities, Match and the newest events as POD rows (D135, D141). Copies only, computes nothing (D37).
    class FlatService
    {
    private:
        using BridgeTick = Platform::Bridge::Type::BridgeTick;

        using Tick = Export::Tick::Type::Tick;

    public:
        FlatService() = default;
        ~FlatService() = default;

        FlatService(const FlatService&) = delete;
        auto operator=(const FlatService&) -> FlatService& = delete;

        // Fills the players, the entities, the match, the events, their counts, the flags and the generation of the tick.
        // note: The clock fields of the header are the bridge's (BridgeService::EndTick).
        auto Flatten(const Tick& tick, BridgeTick& flat) const -> void;
    };
}