export module Export.Tick.Hook:SimulationTicks;

import :ExecuteGameFrame;

import Service.Logs.System;
import Service.Telemetry.State;
import Platform.Memory.System;
import Platform.Lifecycle.State;
import Datum.Object.State;
import Export.Tick.Type;
import std;

export namespace Export::Tick::Hook
{
    // Hooks the engine function that runs the logic ticks of one frame. Marks the engine as running, keeps the engine speed requested by
    // Artemis, records the telemetry of the call and closes its last tick (ExecuteGameFrameDetour closes the others).
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
        using DatumStore = Datum::Object::State::DatumStore;
        using SimulationTicksFunction = Export::Tick::Type::SimulationTicksFunction;

    public:
        SimulationTicksDetour(LogsService& logsService, TelemetryStore& telemetryStore, AOBService& aobService,
            LifecycleStore& lifecycleStore, DatumStore& datumStore, ExecuteGameFrameDetour& executeGameFrameDetour) :
            m_LogsService(logsService), m_TelemetryStore(telemetryStore), m_AOBService(aobService),
            m_LifecycleStore(lifecycleStore), m_DatumStore(datumStore), m_ExecuteGameFrameDetour(executeGameFrameDetour) {}
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
        DatumStore& m_DatumStore;
        ExecuteGameFrameDetour& m_ExecuteGameFrameDetour;

        static inline SimulationTicksDetour* s_Instance{ nullptr };
        static inline std::atomic<int> s_InFlight{ 0 };
        static inline SimulationTicksFunction s_OriginalFunction{ nullptr };

        std::atomic<void*> m_FunctionAddress{ nullptr };
        std::atomic<bool> m_IsHookInstalled{ false };

        // Whether the time scale in the engine is one Artemis wrote. Game thread only.
        bool m_IsTimeScaleOverridden{ false };

        static auto __fastcall HookedSimulationTicks(int ticksToAdvance, float* remainingTime) -> void;

        // Writes the requested speed into the time globals when it differs. With no request, restores 1 once after an override.
        // note: The engine turns the scale into ticks before calling SimulationTicks, so a change applies from the next frame.
        // note: In theater the engine writes its own scale (g_GameTimeScale). A request fights it, so leave it at 0 there.
        auto ApplyTimeScale() -> void;
    };
}