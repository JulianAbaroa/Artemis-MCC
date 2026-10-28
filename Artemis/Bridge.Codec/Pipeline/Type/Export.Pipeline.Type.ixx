export module Export.Pipeline.Type;

import Layout.Snapshot.Type;
import Export.Tick.Type;
import std;

export namespace Export::Pipeline::Type
{
    // What the AI thread hands to the tick observers every tick, after the pipeline and the push to the bridge (when the tick is a decision).
    // note: Delivered on the AI thread, in registration order. An observer must not block.
    struct ObservedTick
    {
        std::shared_ptr<const Export::Tick::Type::Tick> Tick{};
        std::shared_ptr<const Layout::Snapshot::Type::Snapshot> Snapshot{};
        std::uint64_t Generation{ 0 };
        std::uint64_t DroppedBefore{ 0 };
        bool IsFirstAfterLoad{ false };
    };
}