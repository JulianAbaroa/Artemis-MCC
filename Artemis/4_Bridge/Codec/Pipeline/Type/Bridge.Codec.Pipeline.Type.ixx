export module Bridge.Codec.Pipeline.Type;

import Blam.Layout.Snapshot.Type;
import Bridge.Codec.Tick.Type;
import std;

export namespace Bridge::Codec::Pipeline::Type
{
    // What the AI thread hands to the tick observers every tick, after the pipeline and the push to the bridge (when the tick is a decision).
    // note: Delivered on the AI thread, in registration order. An observer must not block.
    struct ObservedTick
    {
        std::shared_ptr<const Bridge::Codec::Tick::Type::Tick> Tick{};
        std::shared_ptr<const Blam::Layout::Snapshot::Type::Snapshot> Snapshot{};
        std::uint64_t Generation{ 0 };
        std::uint64_t DroppedBefore{ 0 };
        bool IsFirstAfterLoad{ false };
    };
}