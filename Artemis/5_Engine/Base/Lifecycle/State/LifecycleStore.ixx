export module Engine.Base.Lifecycle.State:Lifecycle;

import Engine.Base.Lifecycle.Type;
import std;

export namespace Engine::Base::Lifecycle::State
{
    // Lifecycle of the game engine and the synchronization primitives built on it.
    // note: Thread-safe. Written by hooks and the main thread, read by the AI, input and main threads.
    class LifecycleStore
    {
    private:
        using Status = Engine::Base::Lifecycle::Type::Status;

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

        // Signals that the game finished a logic tick, so the AI thread can read a stable object table. While a reader is active,
        // blocks until it ends the read of this tick, so the engine never writes the next tick while Artemis reads this one.
        // return: False if the wait timed out (the tick may be dropped or read mixed with the next one).
        // note: Called from the game main thread through the tick hooks. See docs/Tick Synchronization.md.
        auto SignalTick() -> bool;

        // Sets whether the AI thread is consuming ticks. While false, SignalTick does not wait.
        // note: Setting false wakes the game thread if it is waiting.
        auto SetReaderActive(bool value) -> void;

        // Marks that the AI thread finished reading game memory for a tick, so the game thread can go on.
        // param generation: Tick generation that was read (returned by WaitForTick).
        auto EndRead(std::uint64_t generation) -> void;

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

        // Engine speed Artemis keeps in the time globals (+0x10, D105). 0 means Artemis does not write it and the engine decides.
        // note: Not saved in the settings: Artemis always starts without changing the speed. Written by the UI (later the bridge, D106).
        auto SetRequestedTimeScale(float scale) -> void;
        auto GetRequestedTimeScale() const -> float;

        // Whether the logic ticks are held (D143): frames run without ticks and the Lockstep wait has no cap. Written by the owner of
        // the instance (the UI, or the bridge for the client, D106). Not saved in the settings.
        auto SetPauseRequested(bool value) -> void;
        auto IsPauseRequested() const -> bool;

        // Whether the game thread ends the round on its next frame (D144). Requests not taken yet count as one. Cleared when a match
        // starts or the client stops owning the instance, so a request never reaches another match or owner.
        auto SetEndRoundRequested(bool value) -> void;

        // Takes the pending end of round request.
        // return: Whether there was one.
        // note: Game thread only.
        auto TakeEndRound() -> bool;

        // Frames that ran without ticks because of the pause, since Artemis loaded. Raised by the game thread.
        auto AddPausedFrame() -> void;
        auto GetPausedFrames() const -> std::uint64_t;

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

        std::atomic<bool> m_IsReaderActive{ false };
        // Last tick generation whose read the AI thread ended.
        std::atomic<std::uint64_t> m_ReadGeneration{ 0 };
        std::condition_variable m_ReadCV{};
        std::mutex m_ReadMutex{};

        std::condition_variable m_BlamCV{};
        std::mutex m_BlamMutex{};

        std::atomic<float> m_RequestedTimeScale{ 0.0f };
        std::atomic<bool> m_IsPauseRequested{ false };
        std::atomic<std::uint64_t> m_PausedFrames{ 0 };
        std::atomic<bool> m_IsEndRoundRequested{ false };

        // Wakes every thread blocked on the tick condition variable so they re-check their predicate.
        // note: Takes the mutex first, so a waiter between its check and its wait cannot miss the wake.
        auto WakeTickWaiters() -> void;

        // Same as WakeTickWaiters, for the Blam condition variable.
        auto WakeBlamWaiters() -> void;

        // Same as WakeTickWaiters, for the read condition variable (the game thread waiting in SignalTick).
        auto WakeReadWaiters() -> void;
    };
}