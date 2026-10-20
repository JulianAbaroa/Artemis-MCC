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

    using Platform::Hook::System::k_DrainTimeout;
}

namespace Export::Tick::Hook
{
    auto __fastcall ExecuteGameFrameDetour::HookedExecuteGameFrame() -> void
    {
        Platform::Hook::System::InFlightScope scope{ s_InFlight };

        auto* self{ s_Instance };

        if (self->m_IsInSimulation)
        {
            // The previous tick of this call is complete once the next one starts.
            if (self->m_SimulationTicks > 0) self->CloseTick();
            self->m_SimulationTicks++;
        }

        s_OriginalFunction();
    }

    auto ExecuteGameFrameDetour::Install() -> bool
    {
        if (m_IsHookInstalled.load()) return true;
        s_Instance = this;

        void* functionAddress{ reinterpret_cast<void*>(
            m_AOBService.FindPattern(Signature::k_ExecuteGameFrame)) };
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

        Platform::Hook::System::DisableDetour(m_FunctionAddress.load());
        // note: Uninstalled from RaiseUnhook, which runs on the game thread inside DestroySubsystems, outside the tick loop, so no
        // call of this hook is in flight on that thread. On eject the tick wait wakes with the shutdown signal.
        if (!Platform::Hook::System::WaitForDrain(s_InFlight, k_DrainTimeout))
        {
            m_LogsService.Message("[ExecuteGameFrameDetour] WARNING:"
                " A call was still running when the hook was removed.");
        }
        Platform::Hook::System::RemoveDetour(m_FunctionAddress.load(),
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
        m_SimulationWaitNs = 0;
    }

    auto ExecuteGameFrameDetour::EndSimulation() -> int
    {
        m_IsInSimulation = false;
        return m_SimulationTicks;
    }

    auto ExecuteGameFrameDetour::GetSimulationWaitNs() const -> std::uint64_t
    {
        return m_SimulationWaitNs;
    }

    auto ExecuteGameFrameDetour::CloseTick() -> void
    {
        auto start{ steady_clock::now() };
        const bool isRead{ m_LifecycleStore.SignalTick() };
        auto end{ steady_clock::now() };

        const auto waitNs{ static_cast<std::uint64_t>(std::chrono::duration_cast<nanoseconds>(end - start).count()) };
        m_SimulationWaitNs += waitNs;
        m_TelemetryStore.RecordReadWait(waitNs, !isRead);
    }
}