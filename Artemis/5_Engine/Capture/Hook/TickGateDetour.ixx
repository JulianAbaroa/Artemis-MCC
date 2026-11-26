export module Engine.Capture.Hook:TickGate;

import Common.Log.System;
import Engine.Capture.Type;
import std;

export namespace Engine::Capture::Hook
{
    // Hooks the pause predicate of the engine, but only the call SimulationTicks makes at the start of every tick (TG.1, D380). There it
    // asks the TickGate whether the frame ends before this tick: the engine then cuts its loop as it does for its own pauses and draws.
    // The other callers of the predicate always get the engine answer.
    // note: Without it (signature or bytes not as expected) nothing changes: the game thread waits for Python inside the tick.
    // note: Game main thread only.
    class TickGateDetour
    {
    private:
        using Log = Common::Log::System::Log;
        using PauseCheckFunction = Engine::Capture::Type::PauseCheckFunction;
        using TickGate = Engine::Capture::Type::TickGate;

    public:
        explicit TickGateDetour(Log& log) : m_Log(log) {}
        ~TickGateDetour() = default;

        // Connects the gate. The one that composes the layers calls it before Install. Without a gate the hook returns the engine answer.
        auto SetTickGate(TickGate& tickGate) -> void;

        // Finds the predicate from the call inside SimulationTicks, checks it and installs the hook. Does nothing if already installed.
        // param simulationTicks: Address of SimulationTicks that SimulationTicksDetour found (its start is overwritten by that hook, so it is
        // not scanned again). Null when that hook is not installed.
        // param isTickCounted: Whether ExecuteGameFrameDetour is installed. Without it SimulationTicksDetour counts the ticks it asked for and
        // would produce a tick again on every frame the gate holds, so the gate stays off.
        // note: Any failure logs one ERROR and installs nothing.
        auto Install(void* simulationTicks, bool isTickCounted) -> void;

        // Removes the hook. Does nothing if not installed.
        auto Uninstall() -> void;

    private:
        Log& m_Log;
        TickGate* m_TickGate{ nullptr };

        static inline TickGateDetour* s_Instance{ nullptr };
        static inline std::atomic<int> s_InFlight{ 0 };
        static inline PauseCheckFunction s_OriginalFunction{ nullptr };
        // Return address of the call inside SimulationTicks: the only one that asks the gate.
        static inline std::uintptr_t s_LoopReturn{ 0 };

        std::atomic<void*> m_FunctionAddress{ nullptr };
        std::atomic<bool> m_IsHookInstalled{ false };

        static auto __fastcall HookedPauseCheck() -> bool;
    };
}