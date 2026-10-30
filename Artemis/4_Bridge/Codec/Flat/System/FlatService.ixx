export module Bridge.Codec.Flat.System;

import Contract.Bridge.Type;
import Bridge.Codec.Tick.Type;
import std;

export namespace Bridge::Codec::Flat::System
{
    // Flattens a Tick into a BridgeTick: Players, Entities, Objectives, Stands, Match and the newest events as POD rows (D135, D141). Copies only, computes nothing (D37).
    // The one thing it does is to order the flags and the stands, which Perception keeps by handle, by BaseTeam and handle.
    class FlatService
    {
    private:
        using BridgeTick = Contract::Bridge::Type::BridgeTick;

        using Tick = Bridge::Codec::Tick::Type::Tick;

    public:
        FlatService() = default;
        ~FlatService() = default;

        FlatService(const FlatService&) = delete;
        auto operator=(const FlatService&) -> FlatService& = delete;

        // Fills the players, the entities, the flags, the stands, the match, the events, their counts, the flags and the generation of the tick.
        // note: The clock fields of the header are the bridge's (BridgeService::EndTick).
        auto Flatten(const Tick& tick, BridgeTick& flat) const -> void;
    };
}