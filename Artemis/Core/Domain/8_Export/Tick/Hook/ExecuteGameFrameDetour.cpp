module;

#include "External/minhook/include/MinHook.h"

module Export.Tick.Hook;
import :ExecuteGameFrame;

import Platform.Memory.Type;
import Platform.Hook.System;
import std;

namespace
{
    namespace Signature = Platform::Memory::Type::Signature;
}

namespace Export::Tick::Hook
{
    auto __fastcall ExecuteGameFrameDetour::HookedExecuteGameFrame() -> void
    {
        auto* self{ s_Instance };

        if (self->m_IsInSimulation)
        {
            // The previous tick of this call is complete once the next one starts.
            if (self->m_SimulationTicks > 0) self->m_LifecycleStore.SignalTick();
            self->m_SimulationTicks++;
        }

        s_OriginalFunction();
    }

    auto ExecuteGameFrameDetour::Install() -> bool
    {
        if (m_IsHookInstalled.load()) return true;
        s_Instance = this;

        void* functionAddress{ reinterpret_cast<void*>(
            m_AOBService.FindPattern(Signature::ExecuteGameFrame)) };
        m_FunctionAddress.store(functionAddress);

        if (!Platform::Hook::System::InstallDetour(functionAddress,
            reinterpret_cast<void*>(&HookedExecuteGameFrame),
            reinterpret_cast<void**>(&s_OriginalFunction),
            "[ExecuteGameFrameDetour]", m_LogsService))
        {
            return false;
        }

        m_IsHookInstalled.store(true);
        return true;
    }

    auto ExecuteGameFrameDetour::Uninstall() -> void
    {
        if (!m_IsHookInstalled.load()) return;

        Platform::Hook::System::UninstallDetour(m_FunctionAddress.load(),
            "[ExecuteGameFrameDetour]", m_LogsService);

        m_IsHookInstalled.store(false);
    }

    auto ExecuteGameFrameDetour::IsInstalled() const -> bool
    {
        return m_IsHookInstalled.load();
    }

    auto ExecuteGameFrameDetour::BeginSimulation() -> void
    {
        m_IsInSimulation = true;
        m_SimulationTicks = 0;
    }

    auto ExecuteGameFrameDetour::EndSimulation() -> int
    {
        m_IsInSimulation = false;
        return m_SimulationTicks;
    }
}