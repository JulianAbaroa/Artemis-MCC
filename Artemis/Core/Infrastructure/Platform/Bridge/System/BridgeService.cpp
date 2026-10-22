module;

#include <windows.h>

module Platform.Bridge.System;

import Common.Engine.Type;
import Common.Math.Type;
import Platform.Lifecycle.Type;
import std;

namespace
{
    using Platform::Lifecycle::Type::Status;
    using Platform::Input::Type::ClientRequests;
    using Platform::Input::Type::k_MaxActionPlayers;
    using Platform::Bridge::Type::BridgeOwner;
    using Platform::Bridge::Type::BridgeMode;
    using Platform::Bridge::Type::BridgePhase;
    using Platform::Bridge::Type::BridgeRequest;
    using Platform::Bridge::Type::BridgeCounters;
    using Platform::Bridge::Type::BridgeSlot;

    using std::chrono::steady_clock;
    using std::chrono::nanoseconds;
    using std::chrono::milliseconds;

    using Common::Engine::Type::k_MaxPlayers;
    using Common::Engine::Type::k_NoHandle;
    using Common::Engine::Type::k_HandleIndexMask;
    using Common::Math::Type::k_TwoPi;
    using Common::Math::Type::k_HalfPi;
    using Platform::Bridge::Type::k_BridgeMagic;
    using Platform::Bridge::Type::k_BridgeLayoutVersion;
    using Platform::Bridge::Type::k_BridgeLayoutHash;
    using Platform::Bridge::Type::k_BridgeRegionPrefix;
    using Platform::Bridge::Type::k_BridgeFrameEventSuffix;
    using Platform::Bridge::Type::k_BridgeActionEventSuffix;
    using Platform::Bridge::Type::k_BridgeDiscoveryFolder;
    using Platform::Bridge::Type::k_ClientTimeout;
    using Platform::Bridge::Type::k_LockstepTimeout;
    using Platform::Bridge::Type::k_LockstepWaitSlice;
    using Platform::Bridge::Type::k_MaxLockstepTimeouts;
    using Platform::Bridge::Type::k_MaxDecisionPeriod;
    using Platform::Bridge::Type::k_MinLatency;
    using Platform::Bridge::Type::k_BridgeNotLocal;

    static_assert(k_MaxPlayers <= k_MaxActionPlayers, "Every bridge slot must have an override in the PlayerActionStore.");

    // return: The ASCII name as a wide string (the names of the bridge are ASCII).
    auto Widen(std::string_view text) -> std::wstring
    {
        return std::wstring(text.begin(), text.end());
    }

    // Loads a counter of the region written by the other process.
    auto LoadShared(const std::uint64_t& value) -> std::uint64_t
    {
        // note: atomic_ref of a const object only exists from C++26. The load does not write.
        return std::atomic_ref<std::uint64_t>(const_cast<std::uint64_t&>(value)).load(std::memory_order_acquire);
    }

    auto StoreShared(std::uint64_t& value, std::uint64_t newValue) -> void
    {
        std::atomic_ref<std::uint64_t>(value).store(newValue, std::memory_order_release);
    }

    // return: The value, or 0 when it is not finite. Sets isRejected when it had to change it.
    auto Finite(float value, bool& isRejected) -> float
    {
        if (std::isfinite(value)) return value;
        isRejected = true;
        return 0.0f;
    }

    auto Clamp(float value, float limit, bool& isRejected) -> float
    {
        if (value >= -limit && value <= limit) return value;
        isRejected = true;
        return std::clamp(value, -limit, limit);
    }

    auto ToPhase(Status status) -> BridgePhase
    {
        switch (status)
        {
        case Status::Running: return BridgePhase::Running;
        case Status::Initialized: return BridgePhase::Loading;
        default: return BridgePhase::Idle;
        }
    }
}

namespace Platform::Bridge::System
{
    auto BridgeService::Open() -> bool
    {
        const std::uint32_t pid{ static_cast<std::uint32_t>(GetCurrentProcessId()) };
        const std::string name{ std::string(k_BridgeRegionPrefix) + std::to_string(pid) };
        const std::wstring wideName{ Widen(name) };
        constexpr DWORD size{ static_cast<DWORD>(sizeof(BridgeRegion)) };

        HANDLE mapping{ CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, size, wideName.c_str()) };
        if (mapping == nullptr)
        {
            m_LogsService.Message("[BridgeService] ERROR: Failed to create the shared memory {} (error {}).", name, GetLastError());
            return false;
        }

        void* view{ MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, size) };
        HANDLE frameEvent{ CreateEventW(nullptr, FALSE, FALSE, Widen(name + std::string(k_BridgeFrameEventSuffix)).c_str()) };
        HANDLE actionEvent{ CreateEventW(nullptr, FALSE, FALSE, Widen(name + std::string(k_BridgeActionEventSuffix)).c_str()) };
        if (view == nullptr || frameEvent == nullptr || actionEvent == nullptr)
        {
            m_LogsService.Message("[BridgeService] ERROR: Failed to map the shared memory {} or create its events (error {}).", name,
                GetLastError());
            if (view != nullptr) UnmapViewOfFile(view);
            if (frameEvent != nullptr) CloseHandle(frameEvent);
            if (actionEvent != nullptr) CloseHandle(actionEvent);
            CloseHandle(mapping);
            return false;
        }

        // The region is new for this process: a client that kept an old one open starts over when it sees the new Magic.
        auto* region{ static_cast<BridgeRegion*>(view) };
        std::memset(region, 0, sizeof(BridgeRegion));

        LARGE_INTEGER frequency{};
        QueryPerformanceFrequency(&frequency);

        auto& artemis{ region->Control.Artemis };
        artemis.LayoutVersion = k_BridgeLayoutVersion;
        artemis.LayoutHash = k_BridgeLayoutHash;
        artemis.RegionSize = static_cast<std::uint32_t>(sizeof(BridgeRegion));
        artemis.Pid = pid;
        artemis.QpcFrequency = static_cast<std::uint64_t>(frequency.QuadPart);
        artemis.DecisionPeriod = BridgeControlState{}.DecisionPeriod;
        artemis.Latency = BridgeControlState{}.Latency;

        // Magic goes last: a client that sees it sees the rest of the header.
        std::atomic_ref<std::uint32_t>(artemis.Magic).store(k_BridgeMagic, std::memory_order_release);

        const std::string discovery{ this->WriteDiscovery(pid, name) };
        m_BridgeStore.GetLinkState() = BridgeLinkState{};
        m_BridgeStore.SetRegion(region, mapping, frameEvent, actionEvent, pid, discovery);

        m_LogsService.Message("[BridgeService] INFO: Shared memory {} open ({} bytes).", name, sizeof(BridgeRegion));
        return true;
    }

    auto BridgeService::Close() -> void
    {
        this->Stop();

        BridgeRegion* region{ m_BridgeStore.GetRegion() };
        if (region == nullptr) return;

        std::atomic_ref<std::uint32_t>(region->Control.Artemis.Magic).store(0, std::memory_order_release);
        region->Control.Artemis.Phase = BridgePhase::Idle;

        const std::string discovery{ m_BridgeStore.GetDiscoveryPath() };
        if (!discovery.empty())
        {
            std::error_code error{};
            std::filesystem::remove(discovery, error);
        }

        // Wakes a client waiting for a frame, so it sees Magic 0.
        SetEvent(static_cast<HANDLE>(m_BridgeStore.GetFrameEvent()));

        m_BridgeStore.ClearRegion();
        m_LogsService.Message("[BridgeService] INFO: Shared memory closed.");
    }

    auto BridgeService::Stop() -> void
    {
        m_BridgeStore.SetStopped(true);
    }

    auto BridgeService::BeginMatch() -> void
    {
        m_BridgeStore.GetHookState() = BridgeHookState{};
        m_BridgeStore.SetActionTick(0);
        m_BridgeStore.SetPushedDecision(0);
        for (std::size_t player{ 0 }; player < k_MaxPlayers; ++player) m_BridgeStore.SetAppliedDecision(player, 0);
        m_BridgeStore.SetSlots({});
        m_BridgeStore.SetStopped(false);
        // A request left from the previous match must not end the first round of this one (D144).
        m_LifecycleStore.SetEndRoundRequested(false);
    }

    auto BridgeService::Update() -> void
    {
        BridgeRegion* region{ m_BridgeStore.GetRegion() };
        if (region == nullptr) return;

        auto& link{ m_BridgeStore.GetLinkState() };
        const auto now{ steady_clock::now() };

        const std::uint64_t clientHeartbeat{ LoadShared(region->Control.Client.ClientHeartbeat) };
        if (clientHeartbeat != link.LastClientHeartbeat)
        {
            link.LastClientHeartbeat = clientHeartbeat;
            link.LastClientBeatTime = now;
        }

        // A client that closes writes heartbeat 0: it leaves at once instead of timing out.
        const bool isClientLeft{ clientHeartbeat == 0 };
        const BridgeControlState previous{ m_BridgeStore.GetControl() };
        BridgeControlState control{ this->DecideControl(*region, previous) };
        control.IsClientConnected = !isClientLeft && (now - link.LastClientBeatTime) < k_ClientTimeout;
        if (!control.IsClientConnected)
        {
            control.Owner = BridgeOwner::UI;
            control.Mode = BridgeMode::Free;
            control.AgentMask = 0;
        }

        // The client owns the scale while it owns the instance (D106). The UI gets its own back when the client leaves.
        // The pause follows the same owner (D143).
        if (control.Owner == BridgeOwner::Python && previous.Owner != BridgeOwner::Python)
        {
            link.SavedTimeScale = m_LifecycleStore.GetRequestedTimeScale();
            link.IsSavedPauseRequested = m_LifecycleStore.IsPauseRequested();
        }
        if (control.Owner == BridgeOwner::Python)
        {
            const float scale{ region->Control.Client.RequestScale };
            if (std::isfinite(scale) && scale >= 0.0f && scale != m_LifecycleStore.GetRequestedTimeScale())
            {
                m_LifecycleStore.SetRequestedTimeScale(scale);
            }

            this->ApplyPause(region->Control.Client.RequestPause != 0);
        }
        else if (previous.Owner == BridgeOwner::Python)
        {
            m_LifecycleStore.SetRequestedTimeScale(link.SavedTimeScale);
            this->ApplyPause(link.IsSavedPauseRequested);
            m_LifecycleStore.SetEndRoundRequested(false);
        }

        // A raise of the counter is a request (D144). Raises seen between two updates, or before the game thread takes them, count as
        // one. Seen also without ownership, so it is dropped and not queued.
        const std::uint32_t endRound{ region->Control.Client.RequestEndRound };
        if (endRound != link.LastClientEndRound && control.Owner == BridgeOwner::Python)
        {
            m_LifecycleStore.SetEndRoundRequested(true);
            m_LogsService.Message("[BridgeService] INFO: End of round requested at action tick {}.", m_BridgeStore.GetActionTick());
        }
        link.LastClientEndRound = endRound;

        this->LogTransition(previous, control, isClientLeft);
        m_BridgeStore.SetControl(control);

        link.Heartbeat++;
        this->WriteArtemisControl(*region, control);
    }

    auto BridgeService::CaptureClock(bool isConsecutive, BridgeClock& out) -> void
    {
        out.ActionTick = m_BridgeStore.GetActionTick();

        // Every engine starts its clock again (D139): the first read of a new match is not a skip. Neither is a tick where the engine
        // applied no actions (the action tick repeats, seen when a match ends): it is not a new action tick, so not a decision either.
        const std::uint64_t last{ m_BridgeStore.GetLastReadTick() };
        const std::uint64_t match{ m_MatchCycleStore.GetEngineStarts() };
        const bool isNewMatch{ match != m_BridgeStore.GetLastReadMatch() };
        const bool isRepeated{ !isNewMatch && last != 0 && out.ActionTick == last };
        if (isNewMatch)
        {
            if (last != 0)
            {
                m_LogsService.Message("[BridgeService] INFO: Clock restarted for match {}: action tick {} after {}.", match,
                    out.ActionTick, last);
            }
        }
        else if (isConsecutive && last != 0 && !isRepeated && out.ActionTick != last + 1)
        {
            m_BridgeStore.AddClockSkip();
            m_LogsService.Message("[BridgeService] WARNING: Clock skip in match {}: action tick {} after {}.", match, out.ActionTick,
                last);
        }
        m_BridgeStore.SetLastReadTick(out.ActionTick);
        m_BridgeStore.SetLastReadMatch(match);

        for (std::size_t player{ 0 }; player < k_MaxPlayers; ++player)
        {
            out.AppliedDecisions[player] = m_BridgeStore.GetAppliedDecision(player);
        }

        const BridgeControlState control{ m_BridgeStore.GetControl() };
        out.IsDecision = m_BridgeStore.GetRegion() != nullptr && control.IsClientConnected && out.ActionTick != 0 && !isRepeated &&
            out.ActionTick % control.DecisionPeriod == 0;
    }

    auto BridgeService::ResetClock() -> void
    {
        m_BridgeStore.SetLastReadTick(0);
    }

    auto BridgeService::MarkPushed(std::uint64_t tick) -> void
    {
        m_BridgeStore.SetPushedDecision(tick);
    }

    auto BridgeService::BeginFrame() -> BridgeFrame*
    {
        BridgeRegion* region{ m_BridgeStore.GetRegion() };
        if (region == nullptr) return nullptr;

        m_BridgeStore.GetLinkState().FrameStart = steady_clock::now();

        const std::uint32_t active{ std::atomic_ref<std::uint32_t>(region->Control.Artemis.ActiveBuffer).load(std::memory_order_relaxed) };
        BridgeFrame& frame{ region->Frames[(active + 1) % region->Frames.size()] };

        // Odd sequence: being written. A client that copies the buffer now sees it changed and reads again.
        const std::uint64_t sequence{ frame.Sequence + 1 };
        std::atomic_ref<std::uint64_t>(frame.Sequence).store(sequence, std::memory_order_relaxed);
        std::atomic_ref<std::uint64_t>(frame.SequenceEnd).store(sequence, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_release);

        return &frame;
    }

    auto BridgeService::EndFrame(BridgeFrame& frame, const BridgeClock& clock) -> void
    {
        BridgeRegion* region{ m_BridgeStore.GetRegion() };
        if (region == nullptr) return;

        LARGE_INTEGER counter{};
        QueryPerformanceCounter(&counter);

        frame.Header.MatchId = static_cast<std::uint32_t>(m_MatchCycleStore.GetEngineStarts());
        frame.Header.ActionTick = clock.ActionTick;
        frame.Header.PublishQpc = static_cast<std::uint64_t>(counter.QuadPart);

        // The actions of a slot must carry the handle of its player in this frame (D40).
        std::array<BridgeSlot, k_MaxPlayers> slots{};
        for (std::size_t row{ 0 }; row < frame.Header.PlayerCount; ++row)
        {
            auto& player{ frame.Players[row] };
            const std::size_t index{ player.Handle & k_HandleIndexMask };
            if (index >= k_MaxPlayers) continue;

            player.AppliedDecision = clock.AppliedDecisions[index];
            slots[index].Handle = player.Handle;
            if (player.LocalIndex != k_BridgeNotLocal) slots[index].LocalIndex = player.LocalIndex;
        }
        m_BridgeStore.SetSlots(slots);

        const std::uint64_t sequence{ frame.Sequence + 1 };
        std::atomic_ref<std::uint64_t>(frame.SequenceEnd).store(sequence, std::memory_order_release);
        std::atomic_ref<std::uint64_t>(frame.Sequence).store(sequence, std::memory_order_release);

        const auto index{ static_cast<std::uint32_t>(&frame - region->Frames.data()) };
        std::atomic_ref<std::uint32_t>(region->Control.Artemis.ActiveBuffer).store(index, std::memory_order_release);

        const auto flattenNs{ std::chrono::duration_cast<nanoseconds>(steady_clock::now() - m_BridgeStore.GetLinkState().FrameStart) };
        m_BridgeStore.AddFrame(static_cast<std::uint64_t>(flattenNs.count()));
        BridgeCounters counters{};
        m_BridgeStore.CopyCounters(counters);
        StoreShared(region->Control.Artemis.FrameCount, counters.Frames);

        SetEvent(static_cast<HANDLE>(m_BridgeStore.GetFrameEvent()));
    }

    auto BridgeService::Apply(std::uint64_t actionTick) -> void
    {
        BridgeHookState& hook{ m_BridgeStore.GetHookState() };
        BridgeRegion* region{ m_BridgeStore.GetRegion() };
        const BridgeControlState control{ m_BridgeStore.GetControl() };

        const std::uint32_t agentMask{ region != nullptr && control.Owner == BridgeOwner::Python ? control.AgentMask : 0u };
        this->ReleasePlayers(hook, agentMask);

        if (agentMask != 0)
        {
            std::uint64_t sequence{ LoadShared(region->Control.Client.ActionSequence) };

            // In Lockstep the tick of a decision plus the latency waits for the actions of that decision (D105), if its frame went out.
            // note: The AI thread hands decision t over before it reads tick t + 1, and the game thread waits for that read, so at tick
            // t + d (d >= 2) a decision that will have a frame is always pushed already.
            if (control.Mode == BridgeMode::Lockstep && !hook.IsLockstepSuspended && actionTick >= control.Latency)
            {
                const std::uint64_t due{ (actionTick - control.Latency) / control.DecisionPeriod * control.DecisionPeriod };
                if (due > 0 && due <= m_BridgeStore.GetPushedDecision() && sequence < due)
                {
                    sequence = this->WaitForDecision(*region, due, hook);
                }
            }

            if (hook.IsLockstepSuspended && sequence > hook.AppliedSequence)
            {
                hook.IsLockstepSuspended = false;
                hook.TimeoutsInRow = 0;
                m_LogsService.Message("[BridgeService] INFO: The client answers again: Lockstep resumed.");
            }

            // A decision is applied at its tick plus the latency, or as soon as it arrives when it is late.
            if (sequence > hook.AppliedSequence && sequence + control.Latency <= actionTick)
            {
                if (sequence + control.Latency < actionTick) m_BridgeStore.AddLateAction();
                this->ApplyDecision(*region, sequence, agentMask, hook);
            }

            for (std::size_t player{ 0 }; player < k_MaxPlayers; ++player)
            {
                if ((hook.ControlledMask & (1u << player)) != 0) m_PlayerActionStore.SetOverride(player, hook.Overrides[player]);
            }
        }

        m_BridgeStore.SetActionTick(actionTick);
    }

    auto BridgeService::CopyStatus(BridgeStatus& out) const -> void
    {
        const BridgeControlState control{ m_BridgeStore.GetControl() };
        out.IsOpen = m_BridgeStore.GetRegion() != nullptr;
        out.IsClientConnected = control.IsClientConnected;
        out.Owner = control.Owner;
        out.Mode = control.Mode;
        out.DecisionPeriod = control.DecisionPeriod;
        out.Latency = control.Latency;
        out.AgentMask = control.AgentMask;
        out.Pid = m_BridgeStore.GetPid();
        m_BridgeStore.CopyCounters(out.Counters);
    }

    auto BridgeService::WriteDiscovery(std::uint32_t pid, const std::string& name) -> std::string
    {
        const std::string appData{ m_SettingsStore.GetAppDataDirectory() };
        if (appData.empty())
        {
            m_LogsService.Message("[BridgeService] WARNING: No Artemis folder in LOCALAPPDATA: the client must be given the pid.");
            return {};
        }

        const std::filesystem::path folder{ std::filesystem::path(appData) / k_BridgeDiscoveryFolder };
        const std::filesystem::path path{ folder / (std::to_string(pid) + ".json") };

        std::error_code error{};
        std::filesystem::create_directories(folder, error);

        // The json escapes the backslash of the name.
        std::string escaped{};
        for (const char c : name)
        {
            if (c == '\\') escaped += '\\';
            escaped += c;
        }

        std::ofstream file{ path, std::ios::trunc };
        file << std::format("{{\"pid\": {}, \"region\": \"{}\", \"layoutHash\": \"0x{:016X}\", \"layoutVersion\": {}}}", pid, escaped,
            k_BridgeLayoutHash, k_BridgeLayoutVersion);
        if (!file)
        {
            m_LogsService.Message("[BridgeService] WARNING: Failed to write the discovery file {}.", path.string());
            return {};
        }

        return path.string();
    }

    auto BridgeService::DecideControl(const BridgeRegion& region, const BridgeControlState& previous) -> BridgeControlState
    {
        const auto& client{ region.Control.Client };
        BridgeControlState control{ previous };

        control.Owner = client.RequestOwner == BridgeOwner::Python ? BridgeOwner::Python : BridgeOwner::UI;
        control.Mode = control.Owner == BridgeOwner::Python && client.RequestMode == BridgeMode::Lockstep
            ? BridgeMode::Lockstep : BridgeMode::Free;

        if (client.RequestPeriod != 0) control.DecisionPeriod = std::clamp(client.RequestPeriod, k_MinLatency, k_MaxDecisionPeriod);
        if (client.RequestLatency != 0) control.Latency = client.RequestLatency;
        control.Latency = std::clamp(control.Latency, k_MinLatency, control.DecisionPeriod);

        control.AgentMask = control.Owner == BridgeOwner::Python ? client.AgentMask : 0u;
        return control;
    }

    auto BridgeService::LogTransition(const BridgeControlState& previous, const BridgeControlState& control, bool isClientLeft) -> void
    {
        if (control.IsClientConnected != previous.IsClientConnected)
        {
            if (control.IsClientConnected) m_LogsService.Message("[BridgeService] INFO: Client connected.");
            else if (isClientLeft) m_LogsService.Message("[BridgeService] INFO: Client left.");
            else m_LogsService.Message("[BridgeService] WARNING: Client lost (no heartbeat for {} ms).", k_ClientTimeout.count());
        }

        if (control.Owner != previous.Owner || control.Mode != previous.Mode || control.AgentMask != previous.AgentMask ||
            control.DecisionPeriod != previous.DecisionPeriod || control.Latency != previous.Latency)
        {
            m_LogsService.Message("[BridgeService] INFO: Owner {}, mode {}, N {}, d {}, agents 0x{:04X}.",
                control.Owner == BridgeOwner::Python ? "Python" : "UI", control.Mode == BridgeMode::Lockstep ? "Lockstep" : "Free",
                control.DecisionPeriod, control.Latency, control.AgentMask);
        }
    }

    auto BridgeService::WriteArtemisControl(BridgeRegion& region, const BridgeControlState& control) -> void
    {
        auto& artemis{ region.Control.Artemis };
        BridgeCounters counters{};
        m_BridgeStore.CopyCounters(counters);

        artemis.Phase = ToPhase(m_LifecycleStore.GetStatus());
        artemis.Owner = control.Owner;
        artemis.Mode = control.Mode;
        artemis.DecisionPeriod = control.DecisionPeriod;
        artemis.Latency = control.Latency;
        artemis.IsClientConnected = control.IsClientConnected ? 1 : 0;
        artemis.IsPaused = m_LifecycleStore.IsPauseRequested() ? 1 : 0;
        artemis.MatchId = static_cast<std::uint32_t>(m_MatchCycleStore.GetEngineStarts());
        artemis.Scale = m_LifecycleStore.GetRequestedTimeScale();
        artemis.EndRoundSequence = m_BridgeStore.GetLinkState().LastClientEndRound;

        StoreShared(artemis.ActionTick, m_BridgeStore.GetActionTick());
        StoreShared(artemis.LateActions, counters.LateActions);
        StoreShared(artemis.LockstepWaits, counters.LockstepWaits);
        StoreShared(artemis.LockstepWaitNs, counters.LockstepWaitNs);
        StoreShared(artemis.LockstepTimeouts, counters.LockstepTimeouts);
        StoreShared(artemis.HandleMismatches, counters.HandleMismatches);
        StoreShared(artemis.RejectedActions, counters.RejectedActions);
        StoreShared(artemis.ClockSkips, counters.ClockSkips);
        StoreShared(artemis.LockstepWaitMaxNs, counters.LockstepWaitMaxNs);
        StoreShared(artemis.FlattenNs, counters.FlattenNs);
        StoreShared(artemis.FlattenMaxNs, counters.FlattenMaxNs);
        StoreShared(artemis.PausedFrames, m_LifecycleStore.GetPausedFrames());
        StoreShared(artemis.Heartbeat, m_BridgeStore.GetLinkState().Heartbeat);
    }

    auto BridgeService::ApplyPause(bool isRequested) -> void
    {
        if (isRequested == m_LifecycleStore.IsPauseRequested()) return;

        m_LifecycleStore.SetPauseRequested(isRequested);
        m_LogsService.Message("[BridgeService] INFO: {} at action tick {}.", isRequested ? "Paused" : "Resumed",
            m_BridgeStore.GetActionTick());
    }

    auto BridgeService::WaitForDecision(BridgeRegion& region, std::uint64_t decision, BridgeHookState& hook) -> std::uint64_t
    {
        const auto start{ steady_clock::now() };
        auto deadline{ start + k_LockstepTimeout };
        const auto event{ static_cast<HANDLE>(m_BridgeStore.GetActionEvent()) };
        const auto slice{ static_cast<DWORD>(k_LockstepWaitSlice.count()) };
        bool isPauseSeen{ false };
        bool isClientGone{ false };

        std::uint64_t sequence{ LoadShared(region.Control.Client.ActionSequence) };
        while (sequence < decision && !m_BridgeStore.IsStopped())
        {
            // Only a live client that owns the instance can answer this wait: when it leaves or is lost the game goes on at once.
            const BridgeControlState control{ m_BridgeStore.GetControl() };
            isClientGone = control.Owner != BridgeOwner::Python || !control.IsClientConnected;
            if (isClientGone) break;

            // A pause of the client holds the wait without a cap (D143). The cap starts again when the pause ends.
            const auto now{ steady_clock::now() };
            if (m_LifecycleStore.IsPauseRequested())
            {
                isPauseSeen = true;
                deadline = now + k_LockstepTimeout;
            }
            else if (now >= deadline)
            {
                break;
            }

            WaitForSingleObject(event, slice);
            sequence = LoadShared(region.Control.Client.ActionSequence);
        }

        // A wait that held a pause is pause time, and one cut by a client that left is not a decision: neither is a cost.
        if (!isPauseSeen && !isClientGone)
        {
            const auto waitNs{ std::chrono::duration_cast<nanoseconds>(steady_clock::now() - start).count() };
            m_BridgeStore.AddLockstepWait(static_cast<std::uint64_t>(waitNs));
        }

        if (sequence >= decision)
        {
            hook.TimeoutsInRow = 0;
            return sequence;
        }

        if (m_BridgeStore.IsStopped() || isClientGone) return sequence;

        m_BridgeStore.AddLockstepTimeout();
        if (++hook.TimeoutsInRow >= k_MaxLockstepTimeouts)
        {
            hook.IsLockstepSuspended = true;
            m_LogsService.Message("[BridgeService] WARNING: No actions for decision {} after {} waits of {} ms: Lockstep suspended until "
                "the client answers.", decision, hook.TimeoutsInRow, k_LockstepTimeout.count());
        }

        return sequence;
    }

    auto BridgeService::ApplyDecision(const BridgeRegion& region, std::uint64_t decision, std::uint32_t agentMask,
        BridgeHookState& hook) -> void
    {
        std::array<BridgeSlot, k_MaxPlayers> slots{};
        m_BridgeStore.CopySlots(slots);

        // The turn limit of the client goes with each decision, so all its agents turn alike from the tick it is applied (D114).
        const float turnLimit{ region.Control.Client.RequestTurnLimit };

        for (std::size_t player{ 0 }; player < k_MaxPlayers; ++player)
        {
            if ((agentMask & (1u << player)) == 0) continue;

            // A slot without an answer for this decision keeps its previous action. The client writes DecisionTick 0, the fields, then
            // the decision, so a slot it was writing during the copy is skipped too.
            const auto& slot{ region.Actions[player] };
            if (LoadShared(slot.DecisionTick) != decision) continue;

            BridgeAction action{};
            std::memcpy(&action, &slot, sizeof(action));
            std::atomic_thread_fence(std::memory_order_acquire);
            if (action.DecisionTick != decision || LoadShared(slot.DecisionTick) != decision) continue;

            if (slots[player].Handle == k_NoHandle || action.PlayerHandle != slots[player].Handle)
            {
                m_BridgeStore.AddHandleMismatch();
                if ((hook.ControlledMask & (1u << player)) != 0) this->ReleasePlayer(hook, player);
                continue;
            }

            ActionOverride actionOverride{ this->BuildOverride(action, turnLimit) };
            actionOverride.LocalIndex = slots[player].LocalIndex;
            hook.Overrides[player] = actionOverride;
            hook.ControlledMask |= 1u << player;
            m_BridgeStore.SetAppliedDecision(player, decision);

            const auto requests{ static_cast<std::uint8_t>(action.Requests) };
            if (requests != 0)
            {
                m_PlayerActionStore.RequestClient(player, ClientRequests{
                    .IsWeaponSwitch = (requests & static_cast<std::uint8_t>(BridgeRequest::WeaponSwitch)) != 0,
                    .IsGrenadeSwitch = (requests & static_cast<std::uint8_t>(BridgeRequest::GrenadeSwitch)) != 0,
                    .IsZoom = (requests & static_cast<std::uint8_t>(BridgeRequest::Zoom)) != 0 });
            }
        }

        hook.AppliedSequence = decision;
    }

    auto BridgeService::BuildOverride(const BridgeAction& action, float turnLimit) -> ActionOverride
    {
        bool isRejected{ false };

        // The engine keeps the yaw in [0, 2 pi): any finite yaw is wrapped there.
        float yaw{ std::fmod(Finite(action.Yaw, isRejected), k_TwoPi) };
        if (yaw < 0.0f) yaw += k_TwoPi;

        ActionOverride actionOverride{};
        actionOverride.Buttons = action.Buttons;
        actionOverride.Yaw = yaw;
        actionOverride.Pitch = Clamp(Finite(action.Pitch, isRejected), k_HalfPi, isRejected);
        actionOverride.ThrottleForward = Clamp(Finite(action.ThrottleForward, isRejected), 1.0f, isRejected);
        actionOverride.ThrottleLeft = Clamp(Finite(action.ThrottleLeft, isRejected), 1.0f, isRejected);
        actionOverride.Lanes = action.Lanes;
        actionOverride.TurnLimit = std::isfinite(turnLimit) && turnLimit > 0.0f ? turnLimit : 0.0f;

        if (isRejected) m_BridgeStore.AddRejectedAction();
        return actionOverride;
    }

    auto BridgeService::ReleasePlayers(BridgeHookState& hook, std::uint32_t keepMask) -> void
    {
        const std::uint32_t released{ hook.ControlledMask & ~keepMask };
        if (released == 0) return;

        for (std::size_t player{ 0 }; player < k_MaxPlayers; ++player)
        {
            if ((released & (1u << player)) != 0) this->ReleasePlayer(hook, player);
        }
    }

    auto BridgeService::ReleasePlayer(BridgeHookState& hook, std::size_t player) -> void
    {
        // A local player goes back to its own input. A bot has none: the engine would repeat its last action forever, so it is left
        // standing with the facing it had.
        const auto& last{ hook.Overrides[player] };
        if (last.has_value() && !last->LocalIndex.has_value())
        {
            m_PlayerActionStore.SetOverride(player, ActionOverride{ .Yaw = last->Yaw, .Pitch = last->Pitch });
        }
        else
        {
            m_PlayerActionStore.SetOverride(player, std::nullopt);
        }

        hook.Overrides[player].reset();
        hook.ControlledMask &= ~(1u << player);
        m_BridgeStore.SetAppliedDecision(player, 0);
    }
}