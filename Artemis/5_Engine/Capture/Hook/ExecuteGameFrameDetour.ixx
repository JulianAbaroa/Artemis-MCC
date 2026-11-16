export module Engine.Capture.Hook:ExecuteGameFrame;

import Common.Log.System;
import Services.Telemetry.State;
import Engine.Base.Memory.System;
import Engine.Capture.Type;
import Engine.Capture.Oracle.System;
import std;

export namespace Engine::Capture::Hook
{
    // Hooks the engine function that runs one logic tick, so every tick is copied and queued for the AI thread and not only the last one
    // of a frame (D305).
    // note: SimulationTicks runs ExecuteGameFrame once per tick in a loop. A tick is complete when the next ExecuteGameFrame of the same
    // call starts, so this hook hands the previous one to the tick producer there. SimulationTicksDetour hands the last one when the call
    // returns.
    // note: ExecuteGameFrame has other callers (network code). Outside a SimulationTicks call nothing is counted or produced.
    // note: Producing copies the game on the game thread and can wait for room in the queue, at most k_ReadTimeout (TickProducer).
    // note: Game main thread only. Uninstall runs on that thread too (DestroySubsystems), so no call is in flight then.
    class ExecuteGameFrameDetour
    {
    private:
        using steady_clock = std::chrono::steady_clock;
        using nanoseconds = std::chrono::nanoseconds;

        using Log = Common::Log::System::Log;
        using TelemetryStore = Services::Telemetry::State::TelemetryStore;
        using AOBService = Engine::Base::Memory::System::AOBService;
        using ExecuteGameFrameFunction = Engine::Capture::Type::ExecuteGameFrameFunction;
        using OracleService = Engine::Capture::Oracle::System::OracleService;
        using TickProducer = Engine::Capture::Type::TickProducer;

    public:
        ExecuteGameFrameDetour(Log& log, TelemetryStore& telemetryStore, AOBService& aobService, OracleService& oracleService) :
            m_Log(log), m_TelemetryStore(telemetryStore), m_AOBService(aobService), m_OracleService(oracleService) {}
        ~ExecuteGameFrameDetour() = default;

        // Connects the tick producer (the AI queue, D305) that copies and queues every completed tick.
        // note: The one that composes the layers calls it before Install. Runs once, on the thread that builds the layers. Without a producer
        // the hook only counts ticks.
        auto SetTickProducer(TickProducer& tickProducer) -> void;

        // Finds the target by signature and installs the hook. Does nothing if already installed.
        // return: False if the target is not found or the hook fails.
        auto Install() -> bool;

        // Removes the hook. Does nothing if not installed.
        auto Uninstall() -> void;

        auto IsInstalled() const -> bool;

        // Starts counting the ticks of one SimulationTicks call.
        // note: Called by SimulationTicksDetour before the original runs.
        auto BeginSimulation() -> void;

        // Stops counting.
        // return: Ticks run by the call.
        auto EndSimulation() -> int;

        // Time the game thread spent producing ticks (the capture plus the wait for room in the queue) inside the current or last
        // SimulationTicks call, in nanoseconds.
        auto GetSimulationWaitNs() const -> std::uint64_t;

        // Hands a complete tick to the tick producer and records what it cost the game thread (the read wait: the capture plus the wait for
        // room in the queue), then lets the raycast oracle trace its rays.
        // note: Called here between ticks and by SimulationTicksDetour for the last tick of a call. The copy of the tick is complete when
        // the oracle runs, so it never traces rays against a state the capture has not read yet.
        auto CloseTick() -> void;

        // Hands a complete tick to the tick producer and records its cost, without the oracle.
        // note: SimulationTicksDetour calls it when this hook is not installed.
        auto ProduceTick() -> void;

    private:
        Log& m_Log;
        TelemetryStore& m_TelemetryStore;
        AOBService& m_AOBService;
        OracleService& m_OracleService;
        TickProducer* m_TickProducer{ nullptr };

        static inline ExecuteGameFrameDetour* s_Instance{ nullptr };
        static inline std::atomic<int> s_InFlight{ 0 };
        static inline ExecuteGameFrameFunction s_OriginalFunction{ nullptr };

        std::atomic<void*> m_FunctionAddress{ nullptr };
        std::atomic<bool> m_IsHookInstalled{ false };

        // Whether the game thread is inside a SimulationTicks call.
        bool m_IsInSimulation{ false };
        // Ticks started by the current SimulationTicks call.
        int m_SimulationTicks{ 0 };
        // Read wait (capture plus queue wait) inside the current SimulationTicks call, in nanoseconds.
        std::uint64_t m_SimulationWaitNs{ 0 };

        static auto __fastcall HookedExecuteGameFrame() -> void;
    };
}