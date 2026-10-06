export module Platform.Lifecycle.Hook:DestroySubsystems;

import Service.Logs.System;
import Platform.Memory.System;
import Platform.Lifecycle.Type;
import Platform.Lifecycle.State;
import Platform.Lifecycle.System;
import std;

export namespace Platform::Lifecycle::Hook
{
    // Hooks the engine destruction. Before the original runs, stops Artemis work and raises the unhook and cleanup events.
    // note: Sets the status to TearingDown first and to Destroyed after the original returns.
    class DestroySubsystemsDetour
    {
    private:
        using LogsService = Service::Logs::System::LogsService;
        using AOBService = Platform::Memory::System::AOBService;
        using DestroySubsystemsFunction = Platform::Lifecycle::Type::DestroySubsystemsFunction;
        using LifecycleStore = Platform::Lifecycle::State::LifecycleStore;
        using LifecycleService = Platform::Lifecycle::System::LifecycleService;

    public:
        DestroySubsystemsDetour(LogsService& logsService, AOBService& aobService,
            LifecycleStore& lifecycleStore, LifecycleService& lifecycleService) :
            m_LogsService(logsService), m_AOBService(aobService),
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
        LogsService& m_LogsService;
        AOBService& m_AOBService;
        LifecycleStore& m_LifecycleStore;
        LifecycleService& m_LifecycleService;

        static inline DestroySubsystemsDetour* s_Instance{ nullptr };
        static inline std::atomic<bool> s_InProgress{ false };
        static inline DestroySubsystemsFunction s_OriginalFunction{ nullptr };

        std::atomic<void*> m_FunctionAddress{ nullptr };
        std::atomic<bool> m_IsHookInstalled{ false };

        static auto __fastcall HookedDestroySubsystems() -> void;
    };
}