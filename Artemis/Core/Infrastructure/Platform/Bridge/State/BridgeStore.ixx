export module Platform.Bridge.State;

import Common.Engine.Type;
import Platform.Bridge.Type;
import std;

namespace
{
    using Common::Engine::Type::k_MaxPlayers;
}

export namespace Platform::Bridge::State
{
    // Shared memory of the bridge (D135, D136) and what the threads of Artemis share about it: the control in effect, the clock of
    // the action ticks, the players of the last tick and the counters.
    // note: The region and its handles are set by BridgeService on the bridge thread, and the region pointer is cleared when it
    // closes (the view stays mapped: a hook in flight may still read it). The control and the slots are behind a mutex (a copy per tick). The clock is atomic: the game thread writes
    // it and the AI thread reads it while the game thread waits for the read of the tick.
    class BridgeStore
    {
    private:
        using BridgeRegion = Platform::Bridge::Type::BridgeRegion;
        using BridgeControlState = Platform::Bridge::Type::BridgeControlState;
        using BridgeSlot = Platform::Bridge::Type::BridgeSlot;
        using BridgeHookState = Platform::Bridge::Type::BridgeHookState;
        using BridgeLinkState = Platform::Bridge::Type::BridgeLinkState;
        using BridgeCounters = Platform::Bridge::Type::BridgeCounters;

    public:
        BridgeStore() = default;
        ~BridgeStore() = default;

        BridgeStore(const BridgeStore&) = delete;
        auto operator=(const BridgeStore&) -> BridgeStore& = delete;

        // param mapping, tickEvent, actionEvent: Win32 handles of the region and its two events.
        auto SetRegion(BridgeRegion* region, void* mapping, void* tickEvent, void* actionEvent, std::uint32_t pid,
            std::string discoveryPath) -> void;
        auto ClearRegion() -> void;

        // return: The mapped region, or null when the bridge is closed.
        auto GetRegion() const -> BridgeRegion*;
        auto GetTickEvent() const -> void*;
        auto GetActionEvent() const -> void*;
        auto GetPid() const -> std::uint32_t;
        auto GetDiscoveryPath() const -> std::string;

        auto SetControl(const BridgeControlState& control) -> void;
        auto GetControl() const -> BridgeControlState;

        auto SetSlots(const std::array<BridgeSlot, k_MaxPlayers>& slots) -> void;
        auto CopySlots(std::array<BridgeSlot, k_MaxPlayers>& out) const -> void;

        // Clock of the game thread: the action tick it applied last and the decision applied to each player.
        auto SetActionTick(std::uint64_t tick) -> void;
        auto GetActionTick() const -> std::uint64_t;
        auto SetAppliedDecision(std::size_t player, std::uint64_t decision) -> void;
        auto GetAppliedDecision(std::size_t player) const -> std::uint64_t;

        // Last decision tick the AI thread handed over for publishing. The game thread only waits for decisions up to it, because a tick
        // the AI thread skipped (dropped, teardown) is never published.
        auto SetPushedDecision(std::uint64_t tick) -> void;
        auto GetPushedDecision() const -> std::uint64_t;

        // note: Game thread only.
        auto GetHookState() -> BridgeHookState&;

        // note: Bridge thread only.
        auto GetLinkState() -> BridgeLinkState&;

        // Last action tick the AI thread read and its match, to count the reads where the clock did not advance by one. AI thread only.
        auto GetLastReadTick() const -> std::uint64_t;
        auto SetLastReadTick(std::uint64_t tick) -> void;
        auto GetLastReadMatch() const -> std::uint64_t;
        auto SetLastReadMatch(std::uint64_t match) -> void;

        // Asks the game thread to stop waiting for actions (unhook, shutdown). Cleared when the bridge opens again.
        auto SetStopped(bool value) -> void;
        auto IsStopped() const -> bool;

        // param flattenNs: time spent flattening and publishing the tick. Bridge thread only.
        auto AddPublishedTick(std::uint64_t flattenNs) -> void;
        auto AddLateAction() -> void;
        // note: Game thread only (the maximum is a plain load and store).
        auto AddLockstepWait(std::uint64_t waitNs) -> void;
        auto AddLockstepTimeout() -> void;
        auto AddHandleMismatch() -> void;
        auto AddRejectedAction() -> void;
        auto AddClockSkip() -> void;
        auto CopyCounters(BridgeCounters& out) const -> void;

    private:
        std::atomic<BridgeRegion*> m_Region{ nullptr };
        void* m_Mapping{ nullptr };
        void* m_TickEvent{ nullptr };
        void* m_ActionEvent{ nullptr };
        std::uint32_t m_Pid{ 0 };
        std::string m_DiscoveryPath{};

        BridgeControlState m_Control{};
        std::array<BridgeSlot, k_MaxPlayers> m_Slots{};
        mutable std::mutex m_Mutex{};

        std::atomic<std::uint64_t> m_ActionTick{ 0 };
        std::array<std::atomic<std::uint64_t>, k_MaxPlayers> m_AppliedDecisions{};
        std::atomic<std::uint64_t> m_PushedDecision{ 0 };
        BridgeHookState m_HookState{};
        BridgeLinkState m_LinkState{};
        std::uint64_t m_LastReadTick{ 0 };
        std::uint64_t m_LastReadMatch{ 0 };
        std::atomic<bool> m_IsStopped{ false };

        std::atomic<std::uint64_t> m_PublishedTicks{ 0 };
        std::atomic<std::uint64_t> m_LateActions{ 0 };
        std::atomic<std::uint64_t> m_LockstepWaits{ 0 };
        std::atomic<std::uint64_t> m_LockstepWaitNs{ 0 };
        std::atomic<std::uint64_t> m_LockstepWaitMaxNs{ 0 };
        std::atomic<std::uint64_t> m_LockstepTimeouts{ 0 };
        std::atomic<std::uint64_t> m_HandleMismatches{ 0 };
        std::atomic<std::uint64_t> m_RejectedActions{ 0 };
        std::atomic<std::uint64_t> m_ClockSkips{ 0 };
        std::atomic<std::uint64_t> m_FlattenNs{ 0 };
        std::atomic<std::uint64_t> m_FlattenMaxNs{ 0 };
    };
}