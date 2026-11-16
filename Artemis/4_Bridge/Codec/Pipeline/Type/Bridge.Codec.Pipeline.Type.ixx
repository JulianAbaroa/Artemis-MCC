export module Bridge.Codec.Pipeline.Type;

import Common.State.Type;
import Blam.Layout.Snapshot.Type;
import Bridge.Codec.Flat.Type;
import Bridge.Codec.Tick.Type;
import std;

export namespace Bridge::Codec::Pipeline::Type
{
    // One tick the game thread copied and queued for the AI thread (D305): the Snapshot (full or light), the clock of the bridge read with it
    // and what the AI thread needs to process it in order.
    // note: Produced on the game thread and consumed on the AI thread, in the order of the game.
    struct CapturedTick
    {
        std::shared_ptr<const Blam::Layout::Snapshot::Type::Snapshot> Snapshot{};
        Bridge::Codec::Flat::Type::BridgeClock Clock{};
        // Number of the tick since the map loaded, from 1.
        std::uint64_t Generation{ 0 };
        // Teardown epoch the tick was copied under (LifecycleStore): a tick of an old epoch is never processed against the stores of the new one.
        std::uint64_t Epoch{ 0 };
        // The Snapshot is light: only the flag pass runs (Q101).
        bool IsLight{ false };
        // Ticks lost (not queued) since the previous queued one.
        std::uint64_t DroppedBefore{ 0 };
        bool IsFirstAfterLoad{ false };
    };

    // The queue that carries the ticks from the game thread (producer) to the AI thread (consumer), in order (D305).
    struct TickQueue : Common::State::Type::BoundedQueue<CapturedTick>
    {
        using BoundedQueue::BoundedQueue;
    };

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