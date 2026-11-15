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

    // How long the teardown waits for the load of the Artemis resources.
    constexpr milliseconds k_LoadEndTimeout{ 1000 };
    // How long the teardown waits for the Artemis tick in flight (C.9, D297). The tick stops at the next stage boundary once it sees
    // TearingDown, so it ends within a few milliseconds. The cap only matters if the AI thread hangs. The Cleanup then runs with no stage
    // still reading the stores it empties.
    constexpr milliseconds k_TickEndTimeout{ 1000 };
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

        if (!lifecycle.WaitForLoadEnd(k_LoadEndTimeout))
        {
            logs.Message("[DestroySubsystemsDetour] WARNING:"
                " Artemis resources load did not finish within timeout.");
        }
        if (!lifecycle.WaitForTickEnd(k_TickEndTimeout))
        {
            // The Cleanup runs anyway (the AI thread hangs). The tick that is still running publishes nothing: it sees the state before
            // it publishes (C.7).
            lifecycle.AddTeardownTimeout();
            logs.Message("[DestroySubsystemsDetour] WARNING:"
                " Artemis tick did not finish within timeout.");
        }

        s_Instance->m_LifecycleService.RaiseUnhook();
        s_Instance->m_LifecycleService.RaiseCleanup();

        s_OriginalFunction();

        lifecycle.SetStatus(Status::Destroyed);

        logs.Message("[DestroySubsystemsDetour] INFO: Game engine destroyed."
            " Teardowns with tick timeout: {}. Ticks discarded so far: {}.",
            lifecycle.GetTeardownTimeouts(), lifecycle.GetDiscardedTicks());

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