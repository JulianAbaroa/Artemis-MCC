module Engine.Match.Hook;
import :LifecycleUpdate;

import Engine.Base.Memory.Type;
import Engine.Base.Host.Type;
import Engine.Match.Type;
import Engine.Base.Hook.System;
import std;

namespace
{
    namespace Signature = Engine::Base::Memory::Type::Signature;

    using Engine::Base::Host::Type::k_HostModuleName;

    using Engine::Base::Hook::System::k_DrainTimeout;
    using Engine::Base::Hook::System::ResolveRelative;
    using Engine::Match::Type::k_LifecycleStateOffset;
    using Engine::Match::Type::k_PreGameLifecycleState;
    using Engine::Match::Type::k_LobbyStartIndex;
    using Engine::Match::Type::k_UiManagerDisplacementOffset;
    using Engine::Match::Type::k_UiManagerNextInstructionOffset;
    using Engine::Match::Type::k_LobbyNameDisplacementOffset;
    using Engine::Match::Type::k_LobbyNameNextInstructionOffset;
    using Engine::Match::Type::k_FindScreenDisplacementOffset;
    using Engine::Match::Type::k_FindScreenNextInstructionOffset;
    using Engine::Match::Type::k_SessionKind;
    using Engine::Match::Type::k_GetSessionDisplacementOffset;
    using Engine::Match::Type::k_GetSessionNextInstructionOffset;
    using Engine::Match::Type::k_IsCountdownActiveDisplacementOffset;
    using Engine::Match::Type::k_IsCountdownActiveNextInstructionOffset;
    using Engine::Match::Type::k_EngineUpdateStateIndex;
    using Engine::Match::Type::k_EngineLeaveState;
    using Engine::Match::Type::k_EngineStatePayloadMaxSize;

    using std::chrono::steady_clock;
    using std::chrono::seconds;
    using std::chrono::milliseconds;

    // Frames between two looks for the lobby screen while no launch is pending.
    constexpr std::uint32_t k_LobbyPollFrames{ 15 };

    // How long a launch request waits for the custom games lobby before it is dropped.
    constexpr seconds k_LaunchWaitTimeout{ 180 };

    // How long the lobby screen must be open before START is pressed. Pressed on the frame MCC shows it again after a match, START
    // does nothing (bench, 2026-10-21).
    constexpr milliseconds k_LobbySettleTime{ 3000 };
}

namespace Engine::Match::Hook
{
    auto __fastcall LifecycleUpdateDetour::HookedLifecycleUpdate(void* manager) -> void
    {
        Engine::Base::Hook::System::InFlightScope scope{ s_InFlight };

        auto* self{ s_Instance };

        if (self != nullptr && manager != nullptr)
        {
            if (self->m_MatchConfigStore.IsLaunchRequested())
            {
                self->Launch(manager);
            }
            else
            {
                self->m_IsLaunchWaiting = false;

                if (++self->m_LobbyPollFrame >= k_LobbyPollFrames)
                {
                    self->m_LobbyPollFrame = 0;
                    self->UpdateLobby(manager);
                }
            }

            if (self->m_MatchCycleStore.TakeLeaveRequest()) self->Leave();
        }

        s_OriginalFunction(manager);
    }

    auto __fastcall LifecycleUpdateDetour::HookedVariantsCompatible(std::uint64_t first, std::uint64_t second, std::uint64_t third,
        std::uint64_t fourth) -> bool
    {
        Engine::Base::Hook::System::InFlightScope scope{ s_InFlight };

        auto* self{ s_Instance };

        if (self != nullptr && self->m_IsPressingStart) return true;

        return s_OriginalVariantsCompatible(first, second, third, fourth);
    }

    auto LifecycleUpdateDetour::Install() -> bool
    {
        if (m_IsHookInstalled.load()) return true;
        s_Instance = this;

        const std::uintptr_t lookup{ m_AOBService.FindPattern(Signature::k_FindLobbyScreen, k_HostModuleName) };

        if (lookup == 0)
        {
            m_Log.Message("[LifecycleUpdateDetour] ERROR: Failed to find the lookup of the custom games lobby screen.");
            return false;
        }

        m_UiManagerGlobal = reinterpret_cast<void**>(ResolveRelative(lookup, k_UiManagerDisplacementOffset,
            k_UiManagerNextInstructionOffset));
        m_LobbyScreenName = reinterpret_cast<const char*>(ResolveRelative(lookup, k_LobbyNameDisplacementOffset,
            k_LobbyNameNextInstructionOffset));
        m_FindScreen = reinterpret_cast<FindScreenFunction>(ResolveRelative(lookup, k_FindScreenDisplacementOffset,
            k_FindScreenNextInstructionOffset));

        const std::uintptr_t countdown{ m_AOBService.FindPattern(Signature::k_StartCountdown, k_HostModuleName) };

        if (countdown == 0)
        {
            m_Log.Message("[LifecycleUpdateDetour] ERROR: Failed to find the countdown of the lobby.");
            return false;
        }

        m_GetSession = reinterpret_cast<GetSessionFunction>(ResolveRelative(countdown, k_GetSessionDisplacementOffset,
            k_GetSessionNextInstructionOffset));
        m_IsCountdownActive = reinterpret_cast<IsCountdownActiveFunction>(ResolveRelative(countdown,
            k_IsCountdownActiveDisplacementOffset, k_IsCountdownActiveNextInstructionOffset));

        const std::uintptr_t match{ m_AOBService.FindPattern(Signature::k_LifecycleUpdate, k_HostModuleName) };

        if (match == 0)
        {
            m_Log.Message("[LifecycleUpdateDetour] ERROR: Failed to find the lifecycle update of MCC.");
            return false;
        }

        const std::uintptr_t variants{ m_AOBService.FindPattern(Signature::k_VariantsCompatible, k_HostModuleName) };

        if (variants == 0)
        {
            m_Log.Message("[LifecycleUpdateDetour] ERROR: Failed to find the variants check of the lobby.");
            return false;
        }

        m_VariantsAddress.store(reinterpret_cast<void*>(variants));

        if (!Engine::Base::Hook::System::InstallDetour(reinterpret_cast<void*>(variants),
            reinterpret_cast<void*>(&HookedVariantsCompatible),
            reinterpret_cast<void**>(&s_OriginalVariantsCompatible),
            "[LifecycleUpdateDetour]", m_Log))
        {
            return false;
        }

        void* functionAddress{ reinterpret_cast<void*>(match) };
        m_FunctionAddress.store(functionAddress);

        if (!Engine::Base::Hook::System::InstallDetour(functionAddress,
            reinterpret_cast<void*>(&HookedLifecycleUpdate),
            reinterpret_cast<void**>(&s_OriginalFunction),
            "[LifecycleUpdateDetour]", m_Log))
        {
            // Without the update nothing presses START, so the variants hook goes too.
            Engine::Base::Hook::System::DisableDetour(m_VariantsAddress.load());
            Engine::Base::Hook::System::WaitForDrain(s_InFlight, k_DrainTimeout);
            Engine::Base::Hook::System::RemoveDetour(m_VariantsAddress.load(), "[LifecycleUpdateDetour]", m_Log);
            return false;
        }

        m_IsHookInstalled.store(true);
        return true;
    }

    auto LifecycleUpdateDetour::Uninstall() -> void
    {
        if (!m_IsHookInstalled.load()) return;

        Engine::Base::Hook::System::DisableDetour(m_FunctionAddress.load());
        Engine::Base::Hook::System::DisableDetour(m_VariantsAddress.load());
        Engine::Base::Hook::System::WaitForDrain(s_InFlight, k_DrainTimeout);
        Engine::Base::Hook::System::RemoveDetour(m_FunctionAddress.load(),
            "[LifecycleUpdateDetour]", m_Log);
        Engine::Base::Hook::System::RemoveDetour(m_VariantsAddress.load(),
            "[LifecycleUpdateDetour]", m_Log);

        m_IsHookInstalled.store(false);
        s_Instance = nullptr;
    }

    auto LifecycleUpdateDetour::FindLobby(const void* manager) const -> void*
    {
        std::int32_t state{ 0 };
        std::memcpy(&state, static_cast<const std::byte*>(manager) + k_LifecycleStateOffset, sizeof(state));

        void* uiManager{ *m_UiManagerGlobal };

        return state == k_PreGameLifecycleState && uiManager != nullptr ? m_FindScreen(uiManager, m_LobbyScreenName) : nullptr;
    }

    auto LifecycleUpdateDetour::UpdateLobby(const void* manager) -> void*
    {
        void* screen{ this->FindLobby(manager) };
        const bool isOpen{ screen != nullptr };

        if (isOpen && !m_MatchCycleStore.IsLobbyOpen()) m_LobbyOpenSince = steady_clock::now();
        m_MatchCycleStore.SetLobbyOpen(isOpen);

        return screen;
    }

    auto LifecycleUpdateDetour::Launch(const void* manager) -> void
    {
        void* screen{ this->UpdateLobby(manager) };
        const bool isSettled{ screen != nullptr && steady_clock::now() - m_LobbyOpenSince >= k_LobbySettleTime };

        void* session{ isSettled ? m_GetSession(k_SessionKind) : nullptr };
        const bool isCountingDown{ session != nullptr && m_IsCountdownActive(session) };

        if (!isSettled || isCountingDown)
        {
            if (steady_clock::now() - m_MatchConfigStore.GetLaunchRequestTime() < k_LaunchWaitTimeout)
            {
                if (!m_IsLaunchWaiting)
                {
                    m_IsLaunchWaiting = true;
                    m_MatchConfigStore.SetLastLaunch(isCountingDown ? "Waiting: the lobby is counting down."
                        : "Waiting for the custom games lobby of Reach.");
                }
                return;
            }

            m_IsLaunchWaiting = false;
            m_MatchConfigStore.TakeLaunchRequest();
            m_MatchConfigStore.SetLastLaunch("Dropped: MCC did not reach the custom games lobby in time.");
            m_Log.Message("[LifecycleUpdateDetour] WARNING: MCC did not reach the custom games lobby in {} s, the launch is"
                " dropped.", k_LaunchWaitTimeout.count());
            return;
        }

        m_IsLaunchWaiting = false;
        if (!m_MatchConfigStore.TakeLaunchRequest()) return;

        void** vtable{ *static_cast<void***>(screen) };
        m_IsPressingStart = true;
        reinterpret_cast<LobbyStartFunction>(vtable[k_LobbyStartIndex])(screen);
        m_IsPressingStart = false;

        // START may still refuse with a dialog of its own (too many players): the log only says it was pressed.
        m_MatchConfigStore.SetLastLaunch("START pressed: the lobby should count down and launch.");
        m_Log.Message("[LifecycleUpdateDetour] INFO: START of the lobby pressed.");
    }

    auto LifecycleUpdateDetour::Leave() -> void
    {
        void** global{ m_MatchCycleStore.GetEngineGlobal() };
        void* engine{ global != nullptr ? *global : nullptr };

        if (engine == nullptr)
        {
            m_MatchCycleStore.SetLastLeave("Dropped: no live engine.");
            m_Log.Message("[LifecycleUpdateDetour] WARNING: No live engine, the Leave is dropped.");
            return;
        }

        // The state carries no payload, but the engine queues whatever the pointer holds: it gets zeros.
        const std::array<std::byte, k_EngineStatePayloadMaxSize> payload{};
        void** vtable{ *static_cast<void***>(engine) };
        reinterpret_cast<EngineUpdateStateFunction>(vtable[k_EngineUpdateStateIndex])(engine, k_EngineLeaveState, payload.data());

        m_MatchCycleStore.SetLastLeave("Leave sent to the engine.");
        m_Log.Message("[LifecycleUpdateDetour] INFO: Leave sent to the engine.");
    }
}