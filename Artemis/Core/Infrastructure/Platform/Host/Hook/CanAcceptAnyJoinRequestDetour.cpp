module Platform.Host.Hook;
import :CanAcceptAnyJoinRequest;

import Platform.Memory.Type;
import Platform.Hook.System;
import std;

namespace
{
    namespace Signature = Platform::Memory::Type::Signature;

    using Platform::Hook::System::k_DrainTimeout;
}

namespace Platform::Host::Hook
{
    auto __fastcall CanAcceptAnyJoinRequestDetour::HookedCanAcceptAnyJoinRequest(void* session) -> std::int32_t
    {
        Platform::Hook::System::InFlightScope scope(s_InFlight);

        if (s_Instance->m_LocalUserStore.GetRequestedCount() > 1) return 0;

        return s_OriginalFunction(session);
    }

    auto CanAcceptAnyJoinRequestDetour::Install() -> bool
    {
        if (m_IsHookInstalled.load()) return true;
        s_Instance = this;

        void* functionAddress{ reinterpret_cast<void*>(m_AOBService.FindPattern(Signature::k_CanAcceptAnyJoinRequest)) };
        m_FunctionAddress.store(functionAddress);

        if (!Platform::Hook::System::InstallDetour(functionAddress,
            reinterpret_cast<void*>(&HookedCanAcceptAnyJoinRequest),
            reinterpret_cast<void**>(&s_OriginalFunction),
            "[CanAcceptAnyJoinRequestDetour]", m_LogsService))
        {
            return false;
        }

        m_IsHookInstalled.store(true);
        return true;
    }

    auto CanAcceptAnyJoinRequestDetour::Uninstall() -> void
    {
        if (!m_IsHookInstalled.load()) return;

        Platform::Hook::System::DisableDetour(m_FunctionAddress.load());
        Platform::Hook::System::WaitForDrain(s_InFlight, k_DrainTimeout);
        Platform::Hook::System::RemoveDetour(m_FunctionAddress.load(),
            "[CanAcceptAnyJoinRequestDetour]", m_LogsService);

        m_IsHookInstalled.store(false);
        s_Instance = nullptr;
    }
}