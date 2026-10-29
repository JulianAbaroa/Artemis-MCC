module;

#include <windows.h>
#include "External/minhook/include/MinHook.h"

module Engine.Base.Lifecycle.Hook;
import :DestroySubsystems;

import Engine.Base.Memory.Type;
import Engine.Base.Lifecycle.Type;
import Engine.Base.Hook.System;
import std;

namespace
{
    namespace Signature = Engine::Base::Memory::Type::Signature;
    using Engine::Base::Lifecycle::Type::Status;
    using std::chrono::milliseconds;

    using Engine::Base::Hook::System::k_DrainTimeout;
}

namespace Engine::Base::Lifecycle::Hook
{
    auto DestroySubsystemsDetour::IsInProgress() -> bool
    {
        return s_InProgress.load(std::memory_order_acquire);
    }

    auto __fastcall DestroySubsystemsDetour::HookedDestroySubsystems() -> void
    {
        Engine::Base::Hook::System::InFlightScope scope{ s_InFlight };

        s_InProgress.store(true, std::memory_order_release);

        auto& lifecycle = s_Instance->m_LifecycleStore;
        auto& logs = s_Instance->m_Log;

        lifecycle.SetStatus(Status::TearingDown);

        if (!lifecycle.WaitForLoadEnd(milliseconds(1000)))
        {
            logs.Message("[DestroySubsystemsDetour] WARNING:"
                " Artemis resources load did not finish within timeout.");
        }
        if (!lifecycle.WaitForTickEnd(milliseconds(10)))
        {
            logs.Message("[DestroySubsystemsDetour] WARNING:"
                " Artemis tick did not finish within timeout.");
        }

        s_Instance->m_LifecycleService.RaiseUnhook();
        s_Instance->m_LifecycleService.RaiseCleanup();

        s_OriginalFunction();

        lifecycle.SetStatus(Status::Destroyed);

        logs.Message("[DestroySubsystemsDetour] INFO: Game engine destroyed.");

        s_InProgress.store(false, std::memory_order_release);
    }

    auto DestroySubsystemsDetour::Install() -> bool
    {
        if (m_IsHookInstalled.load()) return true;
        s_Instance = this;

        void* functionAddress = reinterpret_cast<void*>(
            m_AOBService.FindPattern(Signature::k_DestroySubsystems));
        m_FunctionAddress.store(functionAddress);

        if (!Engine::Base::Hook::System::InstallDetour(functionAddress,
            reinterpret_cast<void*>(&HookedDestroySubsystems),
            reinterpret_cast<void**>(&s_OriginalFunction),
            "[DestroySubsystemsDetour]", m_Log))
        {
            return false;
        }

        m_IsHookInstalled.store(true);
        return true;
    }

    auto DestroySubsystemsDetour::Uninstall() -> void
    {
        if (!m_IsHookInstalled.load()) return;

        Engine::Base::Hook::System::DisableDetour(m_FunctionAddress.load());
        if (!Engine::Base::Hook::System::WaitForDrain(s_InFlight, k_DrainTimeout))
        {
            m_Log.Message("[DestroySubsystemsDetour] WARNING:"
                " A call was still running when the hook was removed.");
        }
        Engine::Base::Hook::System::RemoveDetour(m_FunctionAddress.load(),
            "[DestroySubsystemsDetour]", m_Log);

        m_IsHookInstalled.store(false);
    }

    auto DestroySubsystemsDetour::GetFunctionAddress() const -> void*
    {
        return m_FunctionAddress.load();
    }
}