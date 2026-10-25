module;

#include "External/minhook/include/MinHook.h"

module Export.Tick.Hook;
import :SimulationTicks;

import Platform.Memory.Type;
import Platform.Lifecycle.Type;
import Platform.Hook.System;
import Datum.Object.Type;
import Export.Tick.Type;
import std;

namespace
{
    namespace Signature = Platform::Memory::Type::Signature;
    using Platform::Lifecycle::Type::Status;

    using Platform::Hook::System::k_DrainTimeout;
    using Datum::Object::Type::Constant::k_ThreadContextTimeGlobalsOffset;
    using Datum::Object::Type::Constant::k_TimeGlobalsScaleOffset;
    using Export::Tick::Type::k_EndRoundNoWinner;
    using Export::Tick::Type::k_EndRoundScriptReason;

    constexpr float k_DefaultTimeScale{ 1.0f };
}

namespace Export::Tick::Hook
{
    auto __fastcall SimulationTicksDetour::HookedSimulationTicks(int ticksToAdvance, float* remainingTime) -> void
    {
        Platform::Hook::System::InFlightScope scope{ s_InFlight };

        auto* self{ s_Instance };
        auto& frameDetour{ self->m_ExecuteGameFrameDetour };
        const bool isCounting{ frameDetour.IsInstalled() };

        self->m_LifecycleStore.SetStatus(Status::Running);
        self->ApplyTimeScale();
        self->ApplyEndRound();

        // A pause runs the frame without ticks (D143): the engine returns at once when asked for none, and the time left is dropped
        // as its own pause check does, so nothing piles up for the frames after it.
        const bool isPaused{ self->m_LifecycleStore.IsPauseRequested() };
        const int requestedTicks{ isPaused ? 0 : ticksToAdvance };

        if (isCounting) frameDetour.BeginSimulation();

        auto start{ steady_clock::now() };
        s_OriginalFunction(requestedTicks, remainingTime);
        auto end{ steady_clock::now() };

        if (isPaused)
        {
            if (remainingTime != nullptr) *remainingTime = 0.0f;
            self->m_LifecycleStore.AddPausedFrame();
        }

        const auto callNs{ static_cast<std::uint64_t>(std::chrono::duration_cast<nanoseconds>(end - start).count()) };

        if (!isCounting)
        {
            const int ticks{ (std::max)(requestedTicks, 0) };
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

        const std::uintptr_t threadContext{ m_CaptureStore.GetThreadContext() };
        if (threadContext == 0) return;

        const std::uintptr_t timeGlobals{ *reinterpret_cast<const std::uintptr_t*>(threadContext + k_ThreadContextTimeGlobalsOffset) };
        if (timeGlobals == 0) return;

        auto* scale{ reinterpret_cast<float*>(timeGlobals + k_TimeGlobalsScaleOffset) };
        const float target{ requested > 0.0f ? requested : k_DefaultTimeScale };

        if (*scale != target) *scale = target;

        m_IsTimeScaleOverridden = requested > 0.0f;
    }

    auto SimulationTicksDetour::ApplyEndRound() -> void
    {
        if (!m_LifecycleStore.TakeEndRound()) return;

        if (m_EndRound == nullptr || m_CaptureStore.GetThreadContext() == 0)
        {
            m_Log.Message("[SimulationTicksDetour] WARNING: End of round dropped (no engine function or no game).");
            return;
        }

        m_EndRound(k_EndRoundNoWinner, false, k_EndRoundScriptReason);
        m_Log.Message("[SimulationTicksDetour] INFO: Round ended on request.");
    }

    auto SimulationTicksDetour::Install() -> void
    {
        if (m_IsHookInstalled.load()) return;
        s_Instance = this;

        m_EndRound = reinterpret_cast<EndRoundFunction>(m_AOBService.FindPattern(Signature::k_EndRound));

        void* functionAddress{ reinterpret_cast<void*>(
            m_AOBService.FindPattern(Signature::k_SimulationTicks)) };
        m_FunctionAddress.store(functionAddress);

        if (!Platform::Hook::System::InstallDetour(functionAddress,
            reinterpret_cast<void*>(&HookedSimulationTicks),
            reinterpret_cast<void**>(&s_OriginalFunction),
            "[SimulationTicksDetour]", m_Log))
        {
            return;
        }

        m_IsHookInstalled.store(true);
    }

    auto SimulationTicksDetour::Uninstall() -> void
    {
        if (!m_IsHookInstalled.load()) return;

        Platform::Hook::System::DisableDetour(m_FunctionAddress.load());
        // note: Uninstalled from RaiseUnhook, which runs on the game thread inside DestroySubsystems, outside the tick loop, so no
        // call of this hook is in flight on that thread. On eject the tick wait wakes with the shutdown signal.
        if (!Platform::Hook::System::WaitForDrain(s_InFlight, k_DrainTimeout))
        {
            m_Log.Message("[SimulationTicksDetour] WARNING:"
                " A call was still running when the hook was removed.");
        }
        Platform::Hook::System::RemoveDetour(m_FunctionAddress.load(),
            "[SimulationTicksDetour]", m_Log);

        m_IsHookInstalled.store(false);
        s_Instance = nullptr;
    }
}