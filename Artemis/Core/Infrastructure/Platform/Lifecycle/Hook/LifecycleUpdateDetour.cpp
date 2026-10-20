module Platform.Lifecycle.Hook;
import :LifecycleUpdate;

import Platform.Memory.Type;
import Platform.Host.Type;
import Platform.Lifecycle.Type;
import Platform.Hook.System;
import std;

namespace
{
    namespace Signature = Platform::Memory::Type::Signature;

    using Platform::Host::Type::k_HostModuleName;
    using Platform::Hook::System::k_DrainTimeout;
    using Platform::Lifecycle::Type::k_LifecycleStateOffset;
    using Platform::Lifecycle::Type::k_PreGameLifecycleState;
    using Platform::Lifecycle::Type::k_LaunchCountdownSeconds;
}

namespace Platform::Lifecycle::Hook
{
    auto __fastcall LifecycleUpdateDetour::HookedLifecycleUpdate(void* manager) -> void
    {
        Platform::Hook::System::InFlightScope scope{ s_InFlight };

        auto* self{ s_Instance };

        if (self != nullptr && manager != nullptr && self->m_MatchConfigStore.TakeLaunchRequest()) self->Launch(manager);

        s_OriginalFunction(manager);
    }

    auto LifecycleUpdateDetour::Install() -> bool
    {
        if (m_IsHookInstalled.load()) return true;
        s_Instance = this;

        const std::uintptr_t countdown{ m_AOBService.FindPattern(Signature::StartCountdown, k_HostModuleName) };

        if (countdown == 0)
        {
            m_LogsService.Message("[LifecycleUpdateDetour] ERROR: Failed to find the function that starts the countdown of the lobby.");
            return false;
        }

        m_StartCountdown = reinterpret_cast<StartCountdownFunction>(countdown);

        const std::uintptr_t match{ m_AOBService.FindPattern(Signature::LifecycleUpdate, k_HostModuleName) };

        if (match == 0)
        {
            m_LogsService.Message("[LifecycleUpdateDetour] ERROR: Failed to find the lifecycle update of MCC.");
            return false;
        }

        void* functionAddress{ reinterpret_cast<void*>(match) };
        m_FunctionAddress.store(functionAddress);

        if (!Platform::Hook::System::InstallDetour(functionAddress,
            reinterpret_cast<void*>(&HookedLifecycleUpdate),
            reinterpret_cast<void**>(&s_OriginalFunction),
            "[LifecycleUpdateDetour]", m_LogsService))
        {
            return false;
        }

        m_IsHookInstalled.store(true);
        return true;
    }

    auto LifecycleUpdateDetour::Uninstall() -> void
    {
        if (!m_IsHookInstalled.load()) return;

        Platform::Hook::System::DisableDetour(m_FunctionAddress.load());
        Platform::Hook::System::WaitForDrain(s_InFlight, k_DrainTimeout);
        Platform::Hook::System::RemoveDetour(m_FunctionAddress.load(),
            "[LifecycleUpdateDetour]", m_LogsService);

        m_IsHookInstalled.store(false);
        s_Instance = nullptr;
    }

    auto LifecycleUpdateDetour::Launch(const void* manager) -> void
    {
        std::int32_t state{ 0 };
        std::memcpy(&state, static_cast<const std::byte*>(manager) + k_LifecycleStateOffset, sizeof(state));

        if (state != k_PreGameLifecycleState)
        {
            m_MatchConfigStore.SetLastLaunch(std::format("Dropped: MCC is not in a lobby (lifecycle state {}).", state));
            m_LogsService.Message("[LifecycleUpdateDetour] WARNING: MCC is not in a lobby (lifecycle state {}), the launch is dropped.",
                state);
            return;
        }

        if (!m_StartCountdown(k_LaunchCountdownSeconds))
        {
            m_MatchConfigStore.SetLastLaunch("Refused: not the leader of the lobby, or a countdown runs already.");
            m_LogsService.Message("[LifecycleUpdateDetour] WARNING: MCC did not start the countdown of the lobby (not its leader, a"
                " countdown runs already or the session refused it).");
            return;
        }

        m_MatchConfigStore.SetLastLaunch("Launched.");
        m_LogsService.Message("[LifecycleUpdateDetour] INFO: Countdown of the lobby started, the match launches.");
    }
}