export module Platform.Lifecycle.System;

import Service.Logs.System;
import Platform.Lifecycle.Type;
import Platform.Lifecycle.State;
import std;

export namespace Platform::Lifecycle::System
{
    // Raises the engine lifecycle events to the callbacks other layers registered. Also signals shutdown.
    class LifecycleService
    {
    private:
        using LogsService = Service::Logs::System::LogsService;

        using LifecycleCallback = Platform::Lifecycle::Type::LifecycleCallback;

        using LifecycleStore = Platform::Lifecycle::State::LifecycleStore;

    public:
        LifecycleService(LogsService& logsService, LifecycleStore& lifecycleStore) :
            m_LogsService(logsService), m_LifecycleStore(lifecycleStore) {}
        ~LifecycleService() = default;

        // Stops Artemis and wakes every thread waiting for shutdown.
        auto SignalShutdown() -> void;

        // Registers a callback that runs when the game engine finishes initializing.
        auto OnEngineInitialized(LifecycleCallback callback) -> void;

        // Registers a callback that runs before the engine is destroyed, to remove hooks.
        auto OnUnhook(LifecycleCallback callback) -> void;

        // Registers a callback that runs before the engine is destroyed, to clear stored data.
        auto OnCleanup(LifecycleCallback callback) -> void;

        // Registers a callback that runs on the final shutdown of Artemis.
        auto OnShutdown(LifecycleCallback callback) -> void;

        // Runs the engine initialized callbacks in registration order.
        auto RaiseEngineInitialized() -> void;

        // Runs the unhook callbacks in registration order.
        auto RaiseUnhook() -> void;

        // Runs the cleanup callbacks in registration order.
        auto RaiseCleanup() -> void;

        // Runs the shutdown callbacks in reverse registration order.
        // note: Runs only once. Later calls do nothing.
        auto RaiseShutdown() -> void;

    private:
        LogsService& m_LogsService;
        LifecycleStore& m_LifecycleStore;

        std::vector<LifecycleCallback> m_OnEngineInitialized{};
        std::vector<LifecycleCallback> m_OnUnhook{};
        std::vector<LifecycleCallback> m_OnCleanup{};
        std::vector<LifecycleCallback> m_OnShutdown{};

        std::atomic<bool> m_IsShutdownRaised{ false };
    };
}