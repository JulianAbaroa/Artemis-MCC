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

    // Time globals: *(thread context + 0xA0). Speed of the engine: f32 at +0x10 (A8_Velocidad, D105).
    constexpr std::uintptr_t k_TimeGlobalsOffset{ 0xA0 };
    constexpr std::uintptr_t k_TimeScaleOffset{ 0x10 };

    constexpr float k_DefaultTimeScale{ 1.0f };
}

namespace Export::Tick::Hook
{
    auto __fastcall SimulationTicksDetour::HookedSimulationTicks(int ticksToAdvance, float* remainingTime) -> void
    {
        auto* self{ s_Instance };
        auto& frameDetour{ self->m_ExecuteGameFrameDetour };
        const bool isCounting{ frameDetour.IsInstalled() };

        self->m_LifecycleStore.SetStatus(Status::Running);
        self->ApplyTimeScale();

        if (isCounting) frameDetour.BeginSimulation();

        auto start{ steady_clock::now() };
        s_OriginalFunction(ticksToAdvance, remainingTime);
        auto end{ steady_clock::now() };

        const auto callNs{ static_cast<std::uint64_t>(std::chrono::duration_cast<nanoseconds>(end - start).count()) };

        if (!isCounting)
        {
            const int ticks{ (std::max)(ticksToAdvance, 0) };
            self->m_TelemetryStore.RecordSimTick(ticks, callNs);
            if (ticks > 0) self->m_LifecycleStore.SignalTick();
            return;
        }

        // The engine can stop before ticksToAdvance (pause, menus), so the hook count is the real one.
        const int ticks{ frameDetour.EndSimulation() };
        const std::uint64_t waitNs{ frameDetour.GetSimulationWaitNs() };

        self->m_TelemetryStore.RecordSimTick(ticks, callNs > waitNs ? callNs - waitNs : 0);

        // The last tick of the call is complete now.
        if (ticks > 0) frameDetour.CloseTick();
    }

    auto SimulationTicksDetour::ApplyTimeScale() -> void
    {
        const float requested{ m_LifecycleStore.GetRequestedTimeScale() };
        if (requested <= 0.0f && !m_IsTimeScaleOverridden) return;

        const std::uintptr_t threadContext{ m_DatumStore.GetThreadContext() };
        if (threadContext == 0) return;

        const std::uintptr_t timeGlobals{ *reinterpret_cast<const std::uintptr_t*>(threadContext + k_TimeGlobalsOffset) };
        if (timeGlobals == 0) return;

        auto* scale{ reinterpret_cast<float*>(timeGlobals + k_TimeScaleOffset) };
        const float target{ requested > 0.0f ? requested : k_DefaultTimeScale };

        if (*scale != target) *scale = target;

        m_IsTimeScaleOverridden = requested > 0.0f;
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