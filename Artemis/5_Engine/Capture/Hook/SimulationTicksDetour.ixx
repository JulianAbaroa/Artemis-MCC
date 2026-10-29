export module Engine.Capture.Hook:SimulationTicks;

import :ExecuteGameFrame;

import Common.Log.System;
import Services.Telemetry.State;
import Engine.Base.Memory.System;
import Engine.Base.Lifecycle.State;
import Engine.Capture.Type;
import Engine.Capture.State;
import std;

export namespace Engine::Capture::Hook
{
    // Hooks the engine function that runs the logic ticks of one frame. Marks the engine as running, keeps the engine speed requested by
    // Artemis, holds the ticks while a pause is requested (D143), ends the round when asked (D144), records the telemetry of the call
    // and closes its last tick (ExecuteGameFrameDetour closes the others).
    // note: Without the ExecuteGameFrame hook it signals once per call that advanced ticks, so only the last tick of a frame is seen.
    // note: Game main thread only. See docs/Tick Synchronization.md.
    class SimulationTicksDetour
    {
    private:
        using steady_clock = std::chrono::steady_clock;
        using nanoseconds = std::chrono::nanoseconds;

        using Log = Common::Log::System::Log;
        using TelemetryStore = Services::Telemetry::State::TelemetryStore;
        using AOBService = Engine::Base::Memory::System::AOBService;
        using LifecycleStore = Engine::Base::Lifecycle::State::LifecycleStore;
        using CaptureStore = Engine::Capture::State::CaptureStore;
        using SimulationTicksFunction = Engine::Capture::Type::SimulationTicksFunction;
        using EndRoundFunction = Engine::Capture::Type::EndRoundFunction;

    public:
        SimulationTicksDetour(Log& log, TelemetryStore& telemetryStore, AOBService& aobService,
            LifecycleStore& lifecycleStore, CaptureStore& captureStore, ExecuteGameFrameDetour& executeGameFrameDetour) :
            m_Log(log), m_TelemetryStore(telemetryStore), m_AOBService(aobService),
            m_LifecycleStore(lifecycleStore), m_CaptureStore(captureStore), m_ExecuteGameFrameDetour(executeGameFrameDetour) {}
        ~SimulationTicksDetour() = default;

        // Finds the target by signature and installs the hook. Does nothing if already installed.
        auto Install() -> void;

        // Removes the hook. Does nothing if not installed.
        auto Uninstall() -> void;

    private:
        Log& m_Log;
        TelemetryStore& m_TelemetryStore;
        AOBService& m_AOBService;
        LifecycleStore& m_LifecycleStore;
        CaptureStore& m_CaptureStore;
        ExecuteGameFrameDetour& m_ExecuteGameFrameDetour;

        static inline SimulationTicksDetour* s_Instance{ nullptr };
        static inline std::atomic<int> s_InFlight{ 0 };
        static inline SimulationTicksFunction s_OriginalFunction{ nullptr };

        std::atomic<void*> m_FunctionAddress{ nullptr };
        std::atomic<bool> m_IsHookInstalled{ false };

        // Whether the time scale in the engine is one Artemis wrote. Game thread only.
        bool m_IsTimeScaleOverridden{ false };

        // Resolved on Install. Null if its signature was not found.
        EndRoundFunction m_EndRound{ nullptr };

        static auto __fastcall HookedSimulationTicks(int ticksToAdvance, float* remainingTime) -> void;

        // Writes the requested speed into the time globals when it differs. With no request, restores 1 once after an override.
        // note: The engine turns the scale into ticks before calling SimulationTicks, so a change applies from the next frame.
        // note: In theater the engine writes its own scale (g_GameTimeScale). A request fights it, so leave it at 0 there.
        auto ApplyTimeScale() -> void;

        // Ends the round as the end_round action of Megalo does, if a request is pending (D144).
        // note: Runs before the ticks of the frame, also in a pause. The engine ignores it when no round is running.
        auto ApplyEndRound() -> void;
    };
}