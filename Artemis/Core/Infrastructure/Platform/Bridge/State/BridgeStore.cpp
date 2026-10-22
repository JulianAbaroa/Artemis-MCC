module Platform.Bridge.State;

import Common.Engine.Type;
import std;

namespace
{
    using Common::Engine::Type::k_MaxPlayers;
}

namespace Platform::Bridge::State
{
    auto BridgeStore::SetRegion(BridgeRegion* region, void* mapping, void* frameEvent, void* actionEvent, std::uint32_t pid,
        std::string discoveryPath) -> void
    {
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            m_Mapping = mapping;
            m_FrameEvent = frameEvent;
            m_ActionEvent = actionEvent;
            m_Pid = pid;
            m_DiscoveryPath = std::move(discoveryPath);
        }

        m_IsStopped.store(false, std::memory_order_release);
        m_Region.store(region, std::memory_order_release);
    }

    auto BridgeStore::ClearRegion() -> void
    {
        m_Region.store(nullptr, std::memory_order_release);

        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Mapping = nullptr;
        m_FrameEvent = nullptr;
        m_ActionEvent = nullptr;
        m_DiscoveryPath.clear();
        m_Control = {};
        m_Slots = {};
    }

    auto BridgeStore::GetRegion() const -> BridgeRegion*
    {
        return m_Region.load(std::memory_order_acquire);
    }

    auto BridgeStore::GetFrameEvent() const -> void*
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_FrameEvent;
    }

    auto BridgeStore::GetActionEvent() const -> void*
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_ActionEvent;
    }

    auto BridgeStore::GetPid() const -> std::uint32_t
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_Pid;
    }

    auto BridgeStore::GetDiscoveryPath() const -> std::string
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_DiscoveryPath;
    }

    auto BridgeStore::SetControl(const BridgeControlState& control) -> void
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Control = control;
    }

    auto BridgeStore::GetControl() const -> BridgeControlState
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_Control;
    }

    auto BridgeStore::SetSlots(const std::array<BridgeSlot, k_MaxPlayers>& slots) -> void
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Slots = slots;
    }

    auto BridgeStore::CopySlots(std::array<BridgeSlot, k_MaxPlayers>& out) const -> void
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        out = m_Slots;
    }

    auto BridgeStore::SetActionTick(std::uint64_t tick) -> void
    {
        m_ActionTick.store(tick, std::memory_order_release);
    }

    auto BridgeStore::GetActionTick() const -> std::uint64_t
    {
        return m_ActionTick.load(std::memory_order_acquire);
    }

    auto BridgeStore::SetAppliedDecision(std::size_t player, std::uint64_t decision) -> void
    {
        m_AppliedDecisions[player].store(decision, std::memory_order_release);
    }

    auto BridgeStore::GetAppliedDecision(std::size_t player) const -> std::uint64_t
    {
        return m_AppliedDecisions[player].load(std::memory_order_acquire);
    }

    auto BridgeStore::SetPushedDecision(std::uint64_t tick) -> void
    {
        m_PushedDecision.store(tick, std::memory_order_release);
    }

    auto BridgeStore::GetPushedDecision() const -> std::uint64_t
    {
        return m_PushedDecision.load(std::memory_order_acquire);
    }

    auto BridgeStore::GetHookState() -> BridgeHookState&
    {
        return m_HookState;
    }

    auto BridgeStore::GetLinkState() -> BridgeLinkState&
    {
        return m_LinkState;
    }

    auto BridgeStore::GetLastReadTick() const -> std::uint64_t
    {
        return m_LastReadTick;
    }

    auto BridgeStore::SetLastReadTick(std::uint64_t tick) -> void
    {
        m_LastReadTick = tick;
    }

    auto BridgeStore::GetLastReadMatch() const -> std::uint64_t
    {
        return m_LastReadMatch;
    }

    auto BridgeStore::SetLastReadMatch(std::uint64_t match) -> void
    {
        m_LastReadMatch = match;
    }

    auto BridgeStore::SetStopped(bool value) -> void
    {
        m_IsStopped.store(value, std::memory_order_release);
    }

    auto BridgeStore::IsStopped() const -> bool
    {
        return m_IsStopped.load(std::memory_order_acquire);
    }

    auto BridgeStore::AddFrame(std::uint64_t flattenNs) -> void
    {
        m_Frames.fetch_add(1, std::memory_order_relaxed);
        m_FlattenNs.fetch_add(flattenNs, std::memory_order_relaxed);
        if (flattenNs > m_FlattenMaxNs.load(std::memory_order_relaxed)) m_FlattenMaxNs.store(flattenNs, std::memory_order_relaxed);
    }

    auto BridgeStore::AddLateAction() -> void
    {
        m_LateActions.fetch_add(1, std::memory_order_relaxed);
    }

    auto BridgeStore::AddLockstepWait(std::uint64_t waitNs) -> void
    {
        m_LockstepWaits.fetch_add(1, std::memory_order_relaxed);
        m_LockstepWaitNs.fetch_add(waitNs, std::memory_order_relaxed);
        if (waitNs > m_LockstepWaitMaxNs.load(std::memory_order_relaxed)) m_LockstepWaitMaxNs.store(waitNs, std::memory_order_relaxed);
    }

    auto BridgeStore::AddLockstepTimeout() -> void
    {
        m_LockstepTimeouts.fetch_add(1, std::memory_order_relaxed);
    }

    auto BridgeStore::AddHandleMismatch() -> void
    {
        m_HandleMismatches.fetch_add(1, std::memory_order_relaxed);
    }

    auto BridgeStore::AddRejectedAction() -> void
    {
        m_RejectedActions.fetch_add(1, std::memory_order_relaxed);
    }

    auto BridgeStore::AddClockSkip() -> void
    {
        m_ClockSkips.fetch_add(1, std::memory_order_relaxed);
    }

    auto BridgeStore::CopyCounters(BridgeCounters& out) const -> void
    {
        out.Frames = m_Frames.load(std::memory_order_relaxed);
        out.LateActions = m_LateActions.load(std::memory_order_relaxed);
        out.LockstepWaits = m_LockstepWaits.load(std::memory_order_relaxed);
        out.LockstepWaitNs = m_LockstepWaitNs.load(std::memory_order_relaxed);
        out.LockstepWaitMaxNs = m_LockstepWaitMaxNs.load(std::memory_order_relaxed);
        out.LockstepTimeouts = m_LockstepTimeouts.load(std::memory_order_relaxed);
        out.HandleMismatches = m_HandleMismatches.load(std::memory_order_relaxed);
        out.RejectedActions = m_RejectedActions.load(std::memory_order_relaxed);
        out.ClockSkips = m_ClockSkips.load(std::memory_order_relaxed);
        out.FlattenNs = m_FlattenNs.load(std::memory_order_relaxed);
        out.FlattenMaxNs = m_FlattenMaxNs.load(std::memory_order_relaxed);
    }
}