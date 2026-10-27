module;

#include <windows.h>

module Platform.Bridge.System;

import Common.Engine.Type;
import Control.Lockstep.Type;
import Control.Lockstep.System;
import Platform.Lifecycle.Type;
import Layout.Action.Type;
import std;

namespace
{
    using Control::Lockstep::Type::ApplyTiming;
    using Control::Lockstep::Type::BridgeControlState;
    using Control::Lockstep::Type::BridgeHookState;
    using Control::Lockstep::Type::ClockReading;
    using Control::Lockstep::Type::WaitResult;
    using Control::Lockstep::System::ApplyConnection;
    using Control::Lockstep::System::BuildOverride;
    using Control::Lockstep::System::DecideControl;
    using Control::Lockstep::System::GetApplyTiming;
    using Control::Lockstep::System::GetDueDecision;
    using Control::Lockstep::System::GetReleasedOverride;
    using Control::Lockstep::System::IsDecisionTick;
    using Control::Lockstep::System::ReadClock;
    using Control::Lockstep::System::RecordWait;
    using Control::Lockstep::System::ResumeIfAnswered;
    using Control::Lockstep::System::ToClientRequests;

    using Platform::Lifecycle::Type::Status;
    using Platform::Bridge::Type::BridgeOwner;
    using Platform::Bridge::Type::BridgeMode;
    using Platform::Bridge::Type::BridgePhase;
    using Platform::Bridge::Type::BridgeCounters;
    using Platform::Bridge::Type::BridgeSlot;

    using std::chrono::steady_clock;
    using std::chrono::nanoseconds;
    using std::chrono::milliseconds;

    using Common::Engine::Type::k_MaxPlayers;
    using Common::Engine::Type::k_NoHandle;
    using Common::Engine::Type::k_HandleIndexMask;
    using Layout::Action::Type::Size::k_MaxActionPlayers;
    using Platform::Bridge::Type::k_BridgeMagic;
    using Platform::Bridge::Type::k_BridgeLayoutVersion;
    using Platform::Bridge::Type::k_BridgeLayoutHash;
    using Platform::Bridge::Type::k_BridgeRegionPrefix;
    using Platform::Bridge::Type::k_BridgeTickEventSuffix;
    using Platform::Bridge::Type::k_BridgeActionEventSuffix;
    using Platform::Bridge::Type::k_BridgeDiscoveryFolder;
    using Platform::Bridge::Type::k_ClientTimeout;
    using Platform::Bridge::Type::k_LockstepTimeout;
    using Platform::Bridge::Type::k_LockstepWaitSlice;
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
            m_Log.Message("[BridgeService] ERROR: Failed to create the shared memory {} (error {}).", name, GetLastError());
            return false;
        }

        void* view{ MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, size) };
        HANDLE tickEvent{ CreateEventW(nullptr, FALSE, FALSE, Widen(name + std::string(k_BridgeTickEventSuffix)).c_str()) };
        HANDLE actionEvent{ CreateEventW(nullptr, FALSE, FALSE, Widen(name + std::string(k_BridgeActionEventSuffix)).c_str()) };
        if (view == nullptr || tickEvent == nullptr || actionEvent == nullptr)
        {
            m_Log.Message("[BridgeService] ERROR: Failed to map the shared memory {} or create its events (error {}).", name,
                GetLastError());
            if (view != nullptr) UnmapViewOfFile(view);
            if (tickEvent != nullptr) CloseHandle(tickEvent);
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
        m_BridgeStore.SetRegion(region, mapping, tickEvent, actionEvent, pid, discovery);

        m_Log.Message("[BridgeService] INFO: Shared memory {} open ({} bytes).", name, sizeof(BridgeRegion));
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

        // Wakes a client waiting for a tick, so it sees Magic 0.
        SetEvent(static_cast<HANDLE>(m_BridgeStore.GetTickEvent()));

        m_BridgeStore.ClearRegion();
        m_Log.Message("[BridgeService] INFO: Shared memory closed.");
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
        const bool isConnected{ !isClientLeft && (now - link.LastClientBeatTime) < k_ClientTimeout };
        const BridgeControlState control{ ApplyConnection(DecideControl(region->Control.Client, previous), isConnected) };

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
            m_Log.Message("[BridgeService] INFO: End of round requested at action tick {}.", m_BridgeStore.GetActionTick());
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

        // A tick where the engine applied no actions (the action tick repeats, seen when a match ends) is not a new action tick, so
        // not a decision either.
        const std::uint64_t last{ m_BridgeStore.GetLastReadTick() };
        const std::uint64_t match{ m_MatchCycleStore.GetEngineStarts() };
        const ClockReading reading{ ReadClock(out.ActionTick, last, m_BridgeStore.GetLastReadMatch(), match, isConsecutive) };
        if (reading.IsNewMatch)
        {
            if (last != 0)
            {
                m_Log.Message("[BridgeService] INFO: Clock restarted for match {}: action tick {} after {}.", match,
                    out.ActionTick, last);
            }
        }
        else if (reading.IsSkip)
        {
            m_BridgeStore.AddClockSkip();
            m_Log.Message("[BridgeService] WARNING: Clock skip in match {}: action tick {} after {}.", match, out.ActionTick,
                last);
        }
        m_BridgeStore.SetLastReadTick(out.ActionTick);
        m_BridgeStore.SetLastReadMatch(match);

        for (std::size_t player{ 0 }; player < k_MaxPlayers; ++player)
        {
            out.AppliedDecisions[player] = m_BridgeStore.GetAppliedDecision(player);
        }

        const BridgeControlState control{ m_BridgeStore.GetControl() };
        out.IsDecision = IsDecisionTick(m_BridgeStore.GetRegion() != nullptr, control, out.ActionTick, reading.IsRepeated);
    }

    auto BridgeService::ResetClock() -> void
    {
        m_BridgeStore.SetLastReadTick(0);
    }

    auto BridgeService::MarkPushed(std::uint64_t tick) -> void
    {
        m_BridgeStore.SetPushedDecision(tick);
    }

    auto BridgeService::BeginTick() -> BridgeTick*
    {
        BridgeRegion* region{ m_BridgeStore.GetRegion() };
        if (region == nullptr) return nullptr;

        m_BridgeStore.GetLinkState().PublishStart = steady_clock::now();

        const std::uint32_t active{ std::atomic_ref<std::uint32_t>(region->Control.Artemis.ActiveBuffer).load(std::memory_order_relaxed) };
        BridgeTick& flat{ region->Ticks[(active + 1) % region->Ticks.size()] };

        // Odd sequence: being written. A client that copies the buffer now sees it changed and reads again.
        const std::uint64_t sequence{ flat.Sequence + 1 };
        std::atomic_ref<std::uint64_t>(flat.Sequence).store(sequence, std::memory_order_relaxed);
        std::atomic_ref<std::uint64_t>(flat.SequenceEnd).store(sequence, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_release);

        return &flat;
    }

    auto BridgeService::EndTick(BridgeTick& flat, const BridgeClock& clock) -> void
    {
        BridgeRegion* region{ m_BridgeStore.GetRegion() };
        if (region == nullptr) return;

        LARGE_INTEGER counter{};
        QueryPerformanceCounter(&counter);

        flat.Header.MatchId = static_cast<std::uint32_t>(m_MatchCycleStore.GetEngineStarts());
        flat.Header.ActionTick = clock.ActionTick;
        flat.Header.PublishQpc = static_cast<std::uint64_t>(counter.QuadPart);

        // The actions of a slot must carry the handle of its player in this tick (D40).
        std::array<BridgeSlot, k_MaxPlayers> slots{};
        for (std::size_t row{ 0 }; row < flat.Header.PlayerCount; ++row)
        {
            auto& player{ flat.Players[row] };
            const std::size_t index{ player.Handle & k_HandleIndexMask };
            if (index >= k_MaxPlayers) continue;

            player.AppliedDecision = clock.AppliedDecisions[index];
            slots[index].Handle = player.Handle;
            if (player.LocalIndex != k_BridgeNotLocal) slots[index].LocalIndex = player.LocalIndex;
        }
        m_BridgeStore.SetSlots(slots);

        const std::uint64_t sequence{ flat.Sequence + 1 };
        std::atomic_ref<std::uint64_t>(flat.SequenceEnd).store(sequence, std::memory_order_release);
        std::atomic_ref<std::uint64_t>(flat.Sequence).store(sequence, std::memory_order_release);

        const auto index{ static_cast<std::uint32_t>(&flat - region->Ticks.data()) };
        std::atomic_ref<std::uint32_t>(region->Control.Artemis.ActiveBuffer).store(index, std::memory_order_release);

        const auto flattenNs{ std::chrono::duration_cast<nanoseconds>(steady_clock::now() - m_BridgeStore.GetLinkState().PublishStart) };
        m_BridgeStore.AddPublishedTick(static_cast<std::uint64_t>(flattenNs.count()));
        BridgeCounters counters{};
        m_BridgeStore.CopyCounters(counters);
        StoreShared(region->Control.Artemis.PublishedTicks, counters.PublishedTicks);

        SetEvent(static_cast<HANDLE>(m_BridgeStore.GetTickEvent()));
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

            // In Lockstep the tick of a decision plus the latency waits for the actions of that decision (D105), if it was published.
            const std::uint64_t due{ GetDueDecision(actionTick, control, hook, m_BridgeStore.GetPushedDecision(), sequence) };
            if (due != 0) sequence = this->WaitForDecision(*region, due, hook);

            if (ResumeIfAnswered(hook, sequence)) m_Log.Message("[BridgeService] INFO: The client answers again: Lockstep resumed.");

            const ApplyTiming timing{ GetApplyTiming(sequence, hook.AppliedSequence, control.Latency, actionTick) };
            if (timing != ApplyTiming::None)
            {
                if (timing == ApplyTiming::Late) m_BridgeStore.AddLateAction();
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
            m_Log.Message("[BridgeService] WARNING: No Artemis folder in LOCALAPPDATA: the client must be given the pid.");
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
            m_Log.Message("[BridgeService] WARNING: Failed to write the discovery file {}.", path.string());
            return {};
        }

        return path.string();
    }

    auto BridgeService::LogTransition(const BridgeControlState& previous, const BridgeControlState& control, bool isClientLeft) -> void
    {
        if (control.IsClientConnected != previous.IsClientConnected)
        {
            if (control.IsClientConnected) m_Log.Message("[BridgeService] INFO: Client connected.");
            else if (isClientLeft) m_Log.Message("[BridgeService] INFO: Client left.");
            else m_Log.Message("[BridgeService] WARNING: Client lost (no heartbeat for {} ms).", k_ClientTimeout.count());
        }

        if (control.Owner != previous.Owner || control.Mode != previous.Mode || control.AgentMask != previous.AgentMask ||
            control.DecisionPeriod != previous.DecisionPeriod || control.Latency != previous.Latency)
        {
            m_Log.Message("[BridgeService] INFO: Owner {}, mode {}, N {}, d {}, agents 0x{:04X}.",
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
        m_Log.Message("[BridgeService] INFO: {} at action tick {}.", isRequested ? "Paused" : "Resumed",
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

        WaitResult result{ WaitResult::TimedOut };
        if (sequence >= decision) result = WaitResult::Answered;
        else if (m_BridgeStore.IsStopped() || isClientGone) result = WaitResult::Cut;

        if (result == WaitResult::TimedOut) m_BridgeStore.AddLockstepTimeout();
        if (RecordWait(hook, result))
        {
            m_Log.Message("[BridgeService] WARNING: No actions for decision {} after {} waits of {} ms: Lockstep suspended until "
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

            bool isRejected{ false };
            ActionOverride actionOverride{ BuildOverride(action, turnLimit, isRejected) };
            if (isRejected) m_BridgeStore.AddRejectedAction();
            actionOverride.LocalIndex = slots[player].LocalIndex;
            hook.Overrides[player] = actionOverride;
            hook.ControlledMask |= 1u << player;
            m_BridgeStore.SetAppliedDecision(player, decision);

            const auto requests{ static_cast<std::uint8_t>(action.Requests) };
            if (requests != 0)
            {
                m_PlayerActionStore.RequestClient(player, ToClientRequests(requests));
            }
        }

        hook.AppliedSequence = decision;
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
        m_PlayerActionStore.SetOverride(player, GetReleasedOverride(hook.Overrides[player]));

        hook.Overrides[player].reset();
        hook.ControlledMask &= ~(1u << player);
        m_BridgeStore.SetAppliedDecision(player, 0);
    }
}