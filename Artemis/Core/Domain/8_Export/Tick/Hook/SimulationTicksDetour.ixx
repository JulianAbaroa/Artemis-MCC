export module Export.Tick.Hook:SimulationTicks;

import :ExecuteGameFrame;

import Service.Logs.System;
import Service.Telemetry.State;
import Platform.Memory.System;
import Platform.Lifecycle.State;
import Export.Tick.Type;
import std;

export namespace Export::Tick::Hook
{
    // Hooks the engine function that runs the logic ticks of one frame. Marks the engine as running, records the telemetry of the
    // call and signals its last tick (ExecuteGameFrameDetour signals the others).
    // note: Without the ExecuteGameFrame hook it signals once per call that advanced ticks, so only the last tick of a frame is seen.
    // note: Game main thread only. See docs/Tick Synchronization.md.
    class SimulationTicksDetour
    {
    private:
        using steady_clock = std::chrono::steady_clock;
        using nanoseconds = std::chrono::nanoseconds;

        using LogsService = Service::Logs::System::LogsService;
        using TelemetryStore = Service::Telemetry::State::TelemetryStore;
        using AOBService = Platform::Memory::System::AOBService;
        using LifecycleStore = Platform::Lifecycle::State::LifecycleStore;
        using SimulationTicksFunction = Export::Tick::Type::SimulationTicksFunction;

    public:
        SimulationTicksDetour(LogsService& logsService, TelemetryStore& telemetryStore, AOBService& aobService,
            LifecycleStore& lifecycleStore, ExecuteGameFrameDetour& executeGameFrameDetour) :
            m_LogsService(logsService), m_TelemetryStore(telemetryStore), m_AOBService(aobService),
            m_LifecycleStore(lifecycleStore), m_ExecuteGameFrameDetour(executeGameFrameDetour) {}
        ~SimulationTicksDetour() = default;

        // Finds the target by signature and installs the hook. Does nothing if already installed.
        auto Install() -> void;

        // Removes the hook. Does nothing if not installed.
        auto Uninstall() -> void;

    private:
        LogsService& m_LogsService;
        TelemetryStore& m_TelemetryStore;
        AOBService& m_AOBService;
        LifecycleStore& m_LifecycleStore;
        ExecuteGameFrameDetour& m_ExecuteGameFrameDetour;

        static inline SimulationTicksDetour* s_Instance{ nullptr };
        static inline SimulationTicksFunction s_OriginalFunction{ nullptr };

        std::atomic<void*> m_FunctionAddress{ nullptr };
        std::atomic<bool> m_IsHookInstalled{ false };

        static auto __fastcall HookedSimulationTicks(int ticksToAdvance, float* remainingTime) -> void;
    };
}