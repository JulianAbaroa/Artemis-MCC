module Platform.Input.Hook;
import :InputUpdate;

import Platform.Hook.System;
import Platform.Host.Type;
import std;

namespace
{
    using Platform::Input::Type::LocalInputSource;

    using Platform::Hook::System::k_DrainTimeout;

    using Platform::Host::Type::k_InputUpdateIndex;
    using Platform::Host::Type::k_MaxLocalUsers;
}

namespace Platform::Input::Hook
{
    auto __fastcall InputUpdateDetour::HookedInputUpdate(void* gameManager, std::int32_t player, InputState* state) -> bool
    {
        Platform::Hook::System::InFlightScope scope(s_InFlight);

        auto& store{ s_Instance->m_VirtualInputStore };

        const bool isLocalPlayer{ player >= 0 && static_cast<std::size_t>(player) < k_MaxLocalUsers };

        if (!isLocalPlayer || state == nullptr ||
            store.GetSource(static_cast<std::size_t>(player)) != LocalInputSource::Virtual)
        {
            return s_OriginalFunction(gameManager, player, state);
        }

        store.CopyState(static_cast<std::size_t>(player), *state);

        // True tells the engine the state is valid for this frame.
        return true;
    }

    auto InputUpdateDetour::Install() -> bool
    {
        if (m_IsHookInstalled.load()) return true;
        s_Instance = this;

        void* functionAddress{ m_HostService.GetFunctionAddress(k_InputUpdateIndex) };
        m_FunctionAddress.store(functionAddress);

        if (!Platform::Hook::System::InstallDetour(functionAddress,
            reinterpret_cast<void*>(&HookedInputUpdate),
            reinterpret_cast<void**>(&s_OriginalFunction),
            "[InputUpdateDetour]", m_LogsService))
        {
            return false;
        }

        m_IsHookInstalled.store(true);
        return true;
    }

    auto InputUpdateDetour::Uninstall() -> void
    {
        if (!m_IsHookInstalled.load()) return;

        Platform::Hook::System::DisableDetour(m_FunctionAddress.load());
        Platform::Hook::System::WaitForDrain(s_InFlight, k_DrainTimeout);
        Platform::Hook::System::RemoveDetour(m_FunctionAddress.load(),
            "[InputUpdateDetour]", m_LogsService);

        m_IsHookInstalled.store(false);
        s_Instance = nullptr;
    }
}