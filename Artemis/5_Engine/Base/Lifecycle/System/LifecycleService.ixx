export module Platform.Lifecycle.System:Lifecycle;

import Common.Log.System;
import Platform.Lifecycle.Type;
import Platform.Lifecycle.State;
import std;

export namespace Platform::Lifecycle::System
{
    // Raises the engine lifecycle events to the callbacks other layers registered. Also signals shutdown.
    class LifecycleService
    {
    private:
        using Log = Common::Log::System::Log;

        using LifecycleCallback = Platform::Lifecycle::Type::LifecycleCallback;
        using MapFileCallback = Platform::Lifecycle::Type::MapFileCallback;

        using LifecycleStore = Platform::Lifecycle::State::LifecycleStore;

    public:
        LifecycleService(Log& log, LifecycleStore& lifecycleStore) :
            m_Log(log), m_LifecycleStore(lifecycleStore) {}
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

        // Registers a callback that runs when the engine opens a cache file, with the full path of the file.
        auto OnMapFileOpened(MapFileCallback callback) -> void;

        // Runs the engine initialized callbacks in registration order.
        auto RaiseEngineInitialized() -> void;

        // Runs the unhook callbacks in registration order.
        auto RaiseUnhook() -> void;

        // Runs the cleanup callbacks in registration order.
        auto RaiseCleanup() -> void;

        // Runs the map file opened callbacks in registration order.
        // param filePath: Full path of the file the engine opened.
        // note: Runs on the game thread, inside the hook of the engine.
        auto RaiseMapFileOpened(const std::string& filePath) -> void;

        // Runs the shutdown callbacks in reverse registration order.
        // note: Runs only once. Later calls do nothing.
        auto RaiseShutdown() -> void;

    private:
        Log& m_Log;
        LifecycleStore& m_LifecycleStore;

        std::vector<LifecycleCallback> m_OnEngineInitialized{};
        std::vector<LifecycleCallback> m_OnUnhook{};
        std::vector<LifecycleCallback> m_OnCleanup{};
        std::vector<LifecycleCallback> m_OnShutdown{};
        std::vector<MapFileCallback> m_OnMapFileOpened{};

        std::atomic<bool> m_IsShutdownRaised{ false };
    };
}