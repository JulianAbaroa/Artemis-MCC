export module Export.Flat.System;

import Platform.Bridge.Type;
import Export.Tick.Type;
import std;

export namespace Export::Flat::System
{
    // Flattens a Tick into a bridge frame: Players and Entities as POD rows (D135). Copies only, computes nothing (D37).
    class FlatService
    {
    private:
        using BridgeFrame = Platform::Bridge::Type::BridgeFrame;

        using Tick = Export::Tick::Type::Tick;

    public:
        FlatService() = default;
        ~FlatService() = default;

        FlatService(const FlatService&) = delete;
        auto operator=(const FlatService&) -> FlatService& = delete;

        // Fills the players, the entities, their counts, the flags and the generation of the frame.
        // note: The clock fields of the header are the bridge's (BridgeService::EndFrame).
        auto Flatten(const Tick& tick, BridgeFrame& frame) const -> void;
    };
}