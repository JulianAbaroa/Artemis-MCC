export module Platform.Lifecycle.Hook:EngineInitialize;

import Service.Logs.System;
import Platform.Memory.System;
import Platform.Lifecycle.Type;
import Platform.Lifecycle.State;
import Platform.Lifecycle.System;
import std;

export namespace Platform::Lifecycle::Hook
{
    // Hooks the engine initialization. Once the original returns, raises the engine initialized event and sets the status to Initialized.
    class EngineInitializeDetour
    {
    private:
        using LogsService = Service::Logs::System::LogsService;
        using AOBService = Platform::Memory::System::AOBService;
        using EngineInitializeFunction = Platform::Lifecycle::Type::EngineInitializeFunction;
        using LifecycleStore = Platform::Lifecycle::State::LifecycleStore;
        using LifecycleService = Platform::Lifecycle::System::LifecycleService;

    public:
        EngineInitializeDetour(LogsService& logsService, AOBService& aobService,
            LifecycleStore& lifecycleStore, LifecycleService& lifecycleService) :
            m_LogsService(logsService), m_AOBService(aobService),
            m_LifecycleStore(lifecycleStore), m_LifecycleService(lifecycleService) {}
        ~EngineInitializeDetour() = default;

        // Finds the target by signature and installs the hook. Does nothing if already installed.
        // return: False if the target is not found or the hook fails.
        auto Install() -> bool;

        // Removes the hook. Does nothing if not installed.
        auto Uninstall() -> void;

        // Address of the hooked function, or null until installed.
        // note: Used to check that the hook was not overwritten.
        auto GetFunctionAddress() const -> void*;

    private:
        LogsService& m_LogsService;
        AOBService& m_AOBService;
        LifecycleStore& m_LifecycleStore;
        LifecycleService& m_LifecycleService;

        static inline EngineInitializeDetour* s_Instance{ nullptr };
        static inline EngineInitializeFunction s_OriginalFunction{ nullptr };

        std::atomic<void*> m_FunctionAddress{ nullptr };
        std::atomic<bool> m_IsHookInstalled{ false };

        static auto __fastcall HookedEngineInitialize() -> void;
    };
}