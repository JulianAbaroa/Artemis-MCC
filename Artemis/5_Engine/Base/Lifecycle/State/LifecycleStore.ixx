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
        // note: TearingDown raises the teardown epoch. Initialized wakes the Blam waiters.
        auto SetStatus(Status value) -> void;

        // Mutex and condition variable to wait on until shutdown is signaled.
        // note: LifecycleService::SignalShutdown notifies them.
        auto GetShutdownMutex() const -> std::mutex&;
        auto GetShutdownCV() const -> std::condition_variable&;

        // Marks that the AI thread started to process a queued tick, so WaitForTickEnd can wait for it (D305).
        // note: Called right after the tick left the queue, and the AI thread checks the epoch and CanPublish after it: the game thread
        // sets TearingDown and then looks at the flag, so one of the two always sees the other (both sides put a full fence between).
        auto BeginTick() -> void;
        auto EndTick() -> void;

        // Waits for the Artemis tick in progress to finish. The ticks still in the queue are not waited for: the AI thread discards them.
        // return: False if the timeout expired first.
        auto WaitForTickEnd(milliseconds timeout) -> bool;

        // Whether the Artemis tick in progress may still publish its result (the Tick, the bridge frame and the observers).
        // return: False once the engine is tearing down or destroyed, because the Cleanup empties the stores the tick keeps writing.
        // note: Read by the AI thread before it publishes, not only when the tick starts (D271, F1 C.7).
        auto CanPublish() const -> bool;

        // Teardowns that began since Artemis loaded: SetStatus(TearingDown) raises it. A thread that keeps per-load state compares it with the
        // value it last reset for, so it also notices a teardown it slept through or a tick that outlasted it (it saw Destroyed, never
        // TearingDown).
        auto GetTeardownEpoch() const -> std::uint64_t;

        // Teardowns where the Artemis tick did not end within the wait of the game thread, since Artemis loaded. The Cleanup runs
        // anyway, the game thread is never held by the AI thread. Raised by the game thread.
        auto AddTeardownTimeout() -> void;
        auto GetTeardownTimeouts() const -> std::uint64_t;

        // Ticks that were cut because the engine started tearing down while they ran, since Artemis loaded. Raised by the AI thread.
        auto AddDiscardedTick() -> void;
        auto GetDiscardedTicks() const -> std::uint64_t;

        // Publishes that the AI thread loaded the resources of the map for this teardown epoch, so the game thread may start to queue ticks
        // (D305). The first tick queued after it is the first one after the load.
        // param epoch: The teardown epoch the load ran under (GetTeardownEpoch when it began).
        auto MarkLoaded(std::uint64_t epoch) -> void;

        // return: The teardown epoch the resources are loaded for, or the maximum value before any load.
        // note: A game thread that queues ticks compares it with GetTeardownEpoch: a teardown raises the epoch, so the resources of the
        // previous map never match the next one.
        auto GetLoadedEpoch() const -> std::uint64_t;

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

        std::atomic<std::uint64_t> m_LoadedEpoch{ std::numeric_limits<std::uint64_t>::max() };

        std::condition_variable m_BlamCV{};
        std::mutex m_BlamMutex{};

        std::atomic<float> m_RequestedTimeScale{ 0.0f };
        std::atomic<bool> m_IsPauseRequested{ false };
        std::atomic<std::uint64_t> m_PausedFrames{ 0 };
        std::atomic<std::uint64_t> m_TeardownEpoch{ 0 };
        std::atomic<std::uint64_t> m_TeardownTimeouts{ 0 };
        std::atomic<std::uint64_t> m_DiscardedTicks{ 0 };
        std::atomic<bool> m_IsEndRoundRequested{ false };

        // Wakes every thread blocked on the Blam condition variable so they re-check their predicate.
        // note: Takes the mutex first, so a waiter between its check and its wait cannot miss the wake.
        auto WakeBlamWaiters() -> void;
    };
}