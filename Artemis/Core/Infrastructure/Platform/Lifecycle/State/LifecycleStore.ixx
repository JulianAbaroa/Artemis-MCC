export module Platform.Lifecycle.State;

import Platform.Lifecycle.Type;
import std;

export namespace Platform::Lifecycle::State
{
    // Lifecycle of the game engine and the synchronization primitives built on it.
    // note: Thread-safe. Written by hooks and the main thread, read by the AI, input and main threads.
    class LifecycleStore
    {
    private:
        using Status = Platform::Lifecycle::Type::Status;

        using milliseconds = std::chrono::milliseconds;

    public:
        LifecycleStore() = default;
        ~LifecycleStore() = default;

        // Whether Artemis is running.
        auto IsRunning() const -> bool;

        // Sets whether Artemis is running. Setting false wakes every waiting thread.
        auto SetRunning(bool value) -> void;

        auto GetStatus() const -> Status;

        // Changes the engine status.
        // note: TearingDown wakes the tick waiters and Initialized wakes the Blam waiters.
        auto SetStatus(Status value) -> void;

        // Mutex and condition variable to wait on until shutdown is signaled.
        // note: LifecycleService::SignalShutdown notifies them.
        auto GetShutdownMutex() const -> std::mutex&;
        auto GetShutdownCV() const -> std::condition_variable&;

        // Signals that the game finished a logic tick, so the AI thread can read a stable object table.
        // note: Called from the game main thread through the SimulationTicks hook. See docs/Tick Synchronization.md.
        auto SignalTick() -> void;

        // Blocks until a new tick is signaled, Artemis stops, or the engine starts tearing down.
        // param last: Last tick generation the AI thread processed.
        // param dropped: Out. Ticks missed since last, because the AI thread was busy.
        // return: Tick generation the thread woke up on.
        // note: Called from the AI thread. See docs/Tick Synchronization.md.
        auto WaitForTick(std::uint64_t last, std::uint64_t& dropped) -> std::uint64_t;

        auto ResetTickGeneration() -> void;

        // Marks the Artemis tick as running, so WaitForTickEnd can wait for it.
        auto BeginTick() -> void;
        auto EndTick() -> void;

        // Waits for the Artemis tick in progress to finish.
        // return: False if the timeout expired first.
        auto WaitForTickEnd(milliseconds timeout) -> bool;

        // Blocks until the game engine is initialized or Artemis stops.
        auto WaitForBlam() -> void;

        // Marks the resource loading as running, so WaitForLoadEnd can wait for it.
        auto BeginLoad() -> void;
        auto EndLoad() -> void;

        // Waits for the resource loading in progress to finish.
        // return: False if the timeout expired first.
        auto WaitForLoadEnd(milliseconds timeout) -> bool;

    private:
        std::atomic<bool> m_IsRunning{ false };
        std::atomic<Status> m_Status{ Status::Waiting };

        mutable std::condition_variable m_ShutdownCV{};
        mutable std::mutex m_ShutdownMutex{};

        std::atomic<bool> m_IsTickActive{ false };
        std::atomic<bool> m_IsLoadActive{ false };

        std::atomic<std::uint64_t> m_TickGeneration{ 0 };
        std::condition_variable m_TickCV{};
        std::mutex m_TickMutex{};

        std::condition_variable m_BlamCV{};
        std::mutex m_BlamMutex{};

        // Wakes every thread blocked on the tick condition variable so they re-check their predicate.
        // note: Takes the mutex first, so a waiter between its check and its wait cannot miss the wake.
        auto WakeTickWaiters() -> void;

        // Same as WakeTickWaiters, for the Blam condition variable.
        auto WakeBlamWaiters() -> void;
    };
}