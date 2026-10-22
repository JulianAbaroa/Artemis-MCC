export module Export.Flat.Type;

import Platform.Bridge.Type;
import Export.Tick.Type;
import std;

export namespace Export::Flat::Type
{
    // A Tick of a decision tick waiting to be flattened into a BridgeTick, with the clock read with it (D135).
    struct FlatRequest
    {
        std::shared_ptr<const Export::Tick::Type::Tick> Snapshot{};
        Platform::Bridge::Type::BridgeClock Clock{};
    };
}