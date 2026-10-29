module Engine.Base.Host.Hook;
import :CanAcceptAnyJoinRequest;

import Engine.Base.Memory.Type;
import Engine.Base.Hook.System;
import std;

namespace
{
    namespace Signature = Engine::Base::Memory::Type::Signature;

    using Engine::Base::Hook::System::k_DrainTimeout;
}

namespace Engine::Base::Host::Hook
{
    auto __fastcall CanAcceptAnyJoinRequestDetour::HookedCanAcceptAnyJoinRequest(void* session) -> std::int32_t
    {
        Engine::Base::Hook::System::InFlightScope scope(s_InFlight);

        if (s_Instance->m_LocalUserStore.GetRequestedCount() > 1) return 0;

        return s_OriginalFunction(session);
    }

    auto CanAcceptAnyJoinRequestDetour::Install() -> bool
    {
        if (m_IsHookInstalled.load()) return true;
        s_Instance = this;

        void* functionAddress{ reinterpret_cast<void*>(m_AOBService.FindPattern(Signature::k_CanAcceptAnyJoinRequest)) };
        m_FunctionAddress.store(functionAddress);

        if (!Engine::Base::Hook::System::InstallDetour(functionAddress,
            reinterpret_cast<void*>(&HookedCanAcceptAnyJoinRequest),
            reinterpret_cast<void**>(&s_OriginalFunction),
            "[CanAcceptAnyJoinRequestDetour]", m_Log))
        {
            return false;
        }

        m_IsHookInstalled.store(true);
        return true;
    }

    auto CanAcceptAnyJoinRequestDetour::Uninstall() -> void
    {
        if (!m_IsHookInstalled.load()) return;

        Engine::Base::Hook::System::DisableDetour(m_FunctionAddress.load());
        Engine::Base::Hook::System::WaitForDrain(s_InFlight, k_DrainTimeout);
        Engine::Base::Hook::System::RemoveDetour(m_FunctionAddress.load(),
            "[CanAcceptAnyJoinRequestDetour]", m_Log);

        m_IsHookInstalled.store(false);
        s_Instance = nullptr;
    }
}