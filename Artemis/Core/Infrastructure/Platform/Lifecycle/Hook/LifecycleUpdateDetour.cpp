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
    using Platform::Hook::System::ResolveRelative;
    using Platform::Lifecycle::Type::k_LifecycleStateOffset;
    using Platform::Lifecycle::Type::k_PreGameLifecycleState;
    using Platform::Lifecycle::Type::k_LobbyStartIndex;
    using Platform::Lifecycle::Type::k_UiManagerDisplacementOffset;
    using Platform::Lifecycle::Type::k_UiManagerNextInstructionOffset;
    using Platform::Lifecycle::Type::k_LobbyNameDisplacementOffset;
    using Platform::Lifecycle::Type::k_LobbyNameNextInstructionOffset;
    using Platform::Lifecycle::Type::k_FindScreenDisplacementOffset;
    using Platform::Lifecycle::Type::k_FindScreenNextInstructionOffset;
    using Platform::Lifecycle::Type::k_SessionKind;
    using Platform::Lifecycle::Type::k_GetSessionDisplacementOffset;
    using Platform::Lifecycle::Type::k_GetSessionNextInstructionOffset;
    using Platform::Lifecycle::Type::k_IsCountdownActiveDisplacementOffset;
    using Platform::Lifecycle::Type::k_IsCountdownActiveNextInstructionOffset;
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

        const std::uintptr_t lookup{ m_AOBService.FindPattern(Signature::FindLobbyScreen, k_HostModuleName) };

        if (lookup == 0)
        {
            m_LogsService.Message("[LifecycleUpdateDetour] ERROR: Failed to find the lookup of the custom games lobby screen.");
            return false;
        }

        m_UiManagerGlobal = reinterpret_cast<void**>(ResolveRelative(lookup, k_UiManagerDisplacementOffset,
            k_UiManagerNextInstructionOffset));
        m_LobbyScreenName = reinterpret_cast<const char*>(ResolveRelative(lookup, k_LobbyNameDisplacementOffset,
            k_LobbyNameNextInstructionOffset));
        m_FindScreen = reinterpret_cast<FindScreenFunction>(ResolveRelative(lookup, k_FindScreenDisplacementOffset,
            k_FindScreenNextInstructionOffset));

        const std::uintptr_t countdown{ m_AOBService.FindPattern(Signature::StartCountdown, k_HostModuleName) };

        if (countdown == 0)
        {
            m_LogsService.Message("[LifecycleUpdateDetour] ERROR: Failed to find the countdown of the lobby.");
            return false;
        }

        m_GetSession = reinterpret_cast<GetSessionFunction>(ResolveRelative(countdown, k_GetSessionDisplacementOffset,
            k_GetSessionNextInstructionOffset));
        m_IsCountdownActive = reinterpret_cast<IsCountdownActiveFunction>(ResolveRelative(countdown,
            k_IsCountdownActiveDisplacementOffset, k_IsCountdownActiveNextInstructionOffset));

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

        // The main menu is PreGame too: only the open lobby screen tells that MCC is in the custom games lobby.
        void* uiManager{ *m_UiManagerGlobal };
        void* screen{ state == k_PreGameLifecycleState && uiManager != nullptr ? m_FindScreen(uiManager, m_LobbyScreenName) : nullptr };

        if (screen == nullptr)
        {
            m_MatchConfigStore.SetLastLaunch(std::format("Dropped: MCC is not in the custom games lobby (lifecycle state {}).", state));
            m_LogsService.Message("[LifecycleUpdateDetour] WARNING: MCC is not in the custom games lobby (lifecycle state {}), the"
                " launch is dropped.", state);
            return;
        }

        void* session{ m_GetSession(k_SessionKind) };

        if (session != nullptr && m_IsCountdownActive(session))
        {
            m_MatchConfigStore.SetLastLaunch("Dropped: the lobby is already counting down.");
            m_LogsService.Message("[LifecycleUpdateDetour] WARNING: The lobby is already counting down, the launch is dropped.");
            return;
        }

        void** vtable{ *static_cast<void***>(screen) };
        reinterpret_cast<LobbyStartFunction>(vtable[k_LobbyStartIndex])(screen);

        // START may still refuse with a dialog of its own (too many players): the log only says it was pressed.
        m_MatchConfigStore.SetLastLaunch("START pressed: the lobby should count down and launch.");
        m_LogsService.Message("[LifecycleUpdateDetour] INFO: START of the lobby pressed.");
    }
}