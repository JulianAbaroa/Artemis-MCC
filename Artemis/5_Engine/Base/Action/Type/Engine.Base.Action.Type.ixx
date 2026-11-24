export module Engine.Base.Action.Type;

import Blam.Layout.Action.Type;
import std;

namespace
{
    using Blam::Layout::Action::Type::Size::k_MaxActionPlayers;
    using Blam::Layout::Action::Type::Size::k_PlayerActionSize;
}

export namespace Engine::Base::Action::Type
{
    // The actions the simulation applied in one tick, as the owner keeps them: plain data, the layout of the Snapshot region is the
    // capture's (Blam.Layout.Action.Type).
    struct AppliedActions
    {
        // Tick counter of the owner: it rises with every tick whose actions were published, so a repeated value means no new tick.
        std::uint64_t Sequence{ 0 };
        // One bit per player table index with an action this tick.
        std::uint32_t Mask{ 0 };
        // The raw actions in the order of the update queue.
        std::array<std::array<std::uint8_t, k_PlayerActionSize>, k_MaxActionPlayers> Actions{};
    };

    // What the capture asks of the owner of the actions the simulation applies (Engine.Control implements it, Mod connects it with
    // CaptureService::SetAppliedActionsPort): the latest tick it published, which Capture lays out as the region PlayerActions of the
    // Snapshot (D371). Declared here because Engine.Capture and Engine.Control do not see each other.
    // note: Game thread only, between two ticks. The owner publishes the actions of a tick inside it, so the copy is that tick's. A tick
    // that publishes nothing leaves the latest one and its Sequence as they were: the capture tells it by the Sequence.
    class AppliedActionsPort
    {
    public:
        virtual ~AppliedActionsPort() = default;

        // param out: Out. The actions of the latest tick that was published.
        // return: Whether there is one. False before the first tick was published, and nothing is written.
        virtual auto CopyApplied(AppliedActions& out) const -> bool = 0;
    };
}