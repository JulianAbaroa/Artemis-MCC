module;

#include "External/minhook/include/MinHook.h"

module Export.Tick.Hook;
import :SimulationTicks;

import Platform.Memory.Type;
import Platform.Lifecycle.Type;
import Platform.Hook.System;
import std;

namespace
{
    namespace Signature = Platform::Memory::Type::Signature;
    using Platform::Lifecycle::Type::Status;
}

namespace Export::Tick::Hook
{
    auto __fastcall SimulationTicksDetour::HookedSimulationTicks(int ticksToAdvance, float* remainingTime) -> void
    {
        auto* self{ s_Instance };
        auto& frameDetour{ self->m_ExecuteGameFrameDetour };
        const bool isCounting{ frameDetour.IsInstalled() };

        self->m_LifecycleStore.SetStatus(Status::Running);

        if (isCounting) frameDetour.BeginSimulation();

        auto start{ steady_clock::now() };
        s_OriginalFunction(ticksToAdvance, remainingTime);
        auto end{ steady_clock::now() };

        // The engine can stop before ticksToAdvance (pause, menus), so the hook count is the real one.
        const int ticks{ isCounting ? frameDetour.EndSimulation() : (std::max)(ticksToAdvance, 0) };

        self->m_TelemetryStore.RecordSimTick(ticks, static_cast<std::uint64_t>(
            std::chrono::duration_cast<nanoseconds>(end - start).count()));

        // The last tick of the call is complete now.
        if (ticks > 0) self->m_LifecycleStore.SignalTick();
    }

    auto SimulationTicksDetour::Install() -> void
    {
        if (m_IsHookInstalled.load()) return;
        s_Instance = this;

        void* functionAddress{ reinterpret_cast<void*>(
            m_AOBService.FindPattern(Signature::SimulationTicks)) };
        m_FunctionAddress.store(functionAddress);

        if (!Platform::Hook::System::InstallDetour(functionAddress,
            reinterpret_cast<void*>(&HookedSimulationTicks),
            reinterpret_cast<void**>(&s_OriginalFunction),
            "[SimulationTicksDetour]", m_LogsService))
        {
            return;
        }

        m_IsHookInstalled.store(true);
    }

    auto SimulationTicksDetour::Uninstall() -> void
    {
        if (!m_IsHookInstalled.load()) return;

        Platform::Hook::System::UninstallDetour(m_FunctionAddress.load(),
            "[SimulationTicksDetour]", m_LogsService);

        m_IsHookInstalled.store(false);
        s_Instance = nullptr;
    }
}