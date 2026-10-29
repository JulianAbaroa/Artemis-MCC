module;

#include <windows.h>
#include "External/minhook/include/MinHook.h"

module Engine.Base.Lifecycle.Hook;
import :EngineInitialize;

import Engine.Base.Memory.Type;
import Engine.Base.Lifecycle.Type;
import Engine.Base.Hook.System;
import std;

namespace
{
    namespace Signature = Engine::Base::Memory::Type::Signature;
    using Engine::Base::Lifecycle::Type::Status;

    using Engine::Base::Hook::System::k_DrainTimeout;
}

namespace Engine::Base::Lifecycle::Hook
{
    auto __fastcall EngineInitializeDetour::HookedEngineInitialize() -> void
    {
        Engine::Base::Hook::System::InFlightScope scope{ s_InFlight };

        s_OriginalFunction();

        s_Instance->m_LifecycleService.RaiseEngineInitialized();

        s_Instance->m_LifecycleStore.SetStatus(Status::Initialized);

        s_Instance->m_Log.Message("[EngineInitializeDetour] INFO:"
            " Game engine initialized.");
    }

    auto EngineInitializeDetour::Install() -> bool
    {
        if (m_IsHookInstalled.load()) return true;
        s_Instance = this;

        void* functionAddress = reinterpret_cast<void*>(
            m_AOBService.FindPattern(Signature::k_EngineInitialize));
        m_FunctionAddress.store(functionAddress);

        if (!Engine::Base::Hook::System::InstallDetour(functionAddress,
            reinterpret_cast<void*>(&HookedEngineInitialize),
            reinterpret_cast<void**>(&s_OriginalFunction),
            "[EngineInitializeDetour]", m_Log))
        {
            return false;
        }

        m_IsHookInstalled.store(true);
        return true;
    }

    auto EngineInitializeDetour::Uninstall() -> void
    {
        if (!m_IsHookInstalled.load()) return;

        Engine::Base::Hook::System::DisableDetour(m_FunctionAddress.load());
        if (!Engine::Base::Hook::System::WaitForDrain(s_InFlight, k_DrainTimeout))
        {
            m_Log.Message("[EngineInitializeDetour] WARNING:"
                " A call was still running when the hook was removed.");
        }
        Engine::Base::Hook::System::RemoveDetour(m_FunctionAddress.load(),
            "[EngineInitializeDetour]", m_Log);

        m_IsHookInstalled.store(false);
    }

    auto EngineInitializeDetour::GetFunctionAddress() const -> void*
    {
        return m_FunctionAddress.load();
    }
}