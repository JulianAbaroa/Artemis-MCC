export module Engine.Base.Lifecycle.Hook:DestroySubsystems;

import Common.Log.System;
import Engine.Base.Memory.System;
import Engine.Base.Lifecycle.Type;
import Engine.Base.Lifecycle.State;
import Engine.Base.Lifecycle.System;
import std;

export namespace Engine::Base::Lifecycle::Hook
{
    // Hooks the engine destruction. Before the original runs, stops Artemis work and raises the unhook and cleanup events.
    // note: Sets the status to TearingDown first and to Destroyed after the original returns.
    class DestroySubsystemsDetour
    {
    private:
        using Log = Common::Log::System::Log;
        using AOBService = Engine::Base::Memory::System::AOBService;
        using DestroySubsystemsFunction = Engine::Base::Lifecycle::Type::DestroySubsystemsFunction;
        using LifecycleStore = Engine::Base::Lifecycle::State::LifecycleStore;
        using LifecycleService = Engine::Base::Lifecycle::System::LifecycleService;

    public:
        DestroySubsystemsDetour(Log& log, AOBService& aobService,
            LifecycleStore& lifecycleStore, LifecycleService& lifecycleService) :
            m_Log(log), m_AOBService(aobService),
            m_LifecycleStore(lifecycleStore), m_LifecycleService(lifecycleService) {}
        ~DestroySubsystemsDetour() = default;

        // Finds the target by signature and installs the hook. Does nothing if already installed.
        // return: False if the target is not found or the hook fails.
        auto Install() -> bool;

        // Removes the hook. Does nothing if not installed.
        auto Uninstall() -> void;

        // Address of the hooked function, or null until installed.
        // note: Used to check that the hook was not overwritten.
        auto GetFunctionAddress() const -> void*;

        // Whether the hooked function is currently running.
        static auto IsInProgress() -> bool;

    private:
        Log& m_Log;
        AOBService& m_AOBService;
        LifecycleStore& m_LifecycleStore;
        LifecycleService& m_LifecycleService;

        static inline DestroySubsystemsDetour* s_Instance{ nullptr };
        static inline std::atomic<int> s_InFlight{ 0 };
        static inline std::atomic<bool> s_InProgress{ false };
        static inline DestroySubsystemsFunction s_OriginalFunction{ nullptr };

        std::atomic<void*> m_FunctionAddress{ nullptr };
        std::atomic<bool> m_IsHookInstalled{ false };

        static auto __fastcall HookedDestroySubsystems() -> void;
    };
}