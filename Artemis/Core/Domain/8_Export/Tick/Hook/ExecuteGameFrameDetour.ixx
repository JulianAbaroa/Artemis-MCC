export module Export.Tick.Hook:ExecuteGameFrame;

import Service.Logs.System;
import Platform.Memory.System;
import Platform.Lifecycle.State;
import Export.Tick.Type;
import std;

export namespace Export::Tick::Hook
{
    // Hooks the engine function that runs one logic tick, so the AI thread is signaled for every tick and not only for the last one
    // of a frame.
    // note: SimulationTicks runs ExecuteGameFrame once per tick in a loop. A tick is complete when the next ExecuteGameFrame of the same
    // call starts, so this hook signals the previous one there. SimulationTicksDetour signals the last one when the call returns.
    // note: ExecuteGameFrame has other callers (network code). Outside a SimulationTicks call nothing is counted or signaled.
    // note: Game main thread only. Uninstall runs on that thread too (DestroySubsystems), so no call is in flight then.
    class ExecuteGameFrameDetour
    {
    private:
        using LogsService = Service::Logs::System::LogsService;
        using AOBService = Platform::Memory::System::AOBService;
        using LifecycleStore = Platform::Lifecycle::State::LifecycleStore;
        using ExecuteGameFrameFunction = Export::Tick::Type::ExecuteGameFrameFunction;

    public:
        ExecuteGameFrameDetour(LogsService& logsService, AOBService& aobService, LifecycleStore& lifecycleStore) :
            m_LogsService(logsService), m_AOBService(aobService), m_LifecycleStore(lifecycleStore) {}
        ~ExecuteGameFrameDetour() = default;

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

    private:
        LogsService& m_LogsService;
        AOBService& m_AOBService;
        LifecycleStore& m_LifecycleStore;

        static inline ExecuteGameFrameDetour* s_Instance{ nullptr };
        static inline ExecuteGameFrameFunction s_OriginalFunction{ nullptr };

        std::atomic<void*> m_FunctionAddress{ nullptr };
        std::atomic<bool> m_IsHookInstalled{ false };

        // Whether the game thread is inside a SimulationTicks call.
        bool m_IsInSimulation{ false };
        // Ticks started by the current SimulationTicks call.
        int m_SimulationTicks{ 0 };

        static auto __fastcall HookedExecuteGameFrame() -> void;
    };
}