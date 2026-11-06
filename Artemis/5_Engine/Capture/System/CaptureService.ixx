export module Engine.Capture.System;

import Common.Log.System;
import Services.Telemetry.State;
import Engine.Base.Lifecycle.State;
import Engine.Base.Memory.System;
import Engine.Capture.State;
import Blam.Layout.Snapshot.Type;
import std;

export namespace Engine::Capture::System
{
    // Copies everything the tick pipeline reads from the game into a snapshot: the live objects and the regions of memory.
    class CaptureService
    {
    private:
        using Log = Common::Log::System::Log;

        using TelemetryStore = Services::Telemetry::State::TelemetryStore;

        using LifecycleStore = Engine::Base::Lifecycle::State::LifecycleStore;
        using MemoryReaderService = Engine::Base::Memory::System::MemoryReaderService;

        using CaptureStore = Engine::Capture::State::CaptureStore;

        using RegionId = Blam::Layout::Snapshot::Type::RegionId;
        using Snapshot = Blam::Layout::Snapshot::Type::Snapshot;

    public:
        CaptureService(Log& log,
            LifecycleStore& lifecycleStore,
            MemoryReaderService& memoryReaderService,
            CaptureStore& captureStore,
            TelemetryStore& telemetryStore) :
            m_Log(log), m_LifecycleStore(lifecycleStore), m_MemoryReaderService(memoryReaderService),
            m_CaptureStore(captureStore), m_TelemetryStore(telemetryStore) {}
        ~CaptureService() = default;

        // Copies everything the tick pipeline reads from the game (objects, placements and the regions of RegionId) into a new
        // Snapshot and publishes it.
        // return: The published snapshot. It is also returned without the thread context, with the flags off and nothing copied.
        // note: Runs on the AI thread inside the read window, the game thread waits for it.
        auto Capture() -> std::shared_ptr<const Snapshot>;

        // Clears the stored addresses and snapshot.
        auto Cleanup() -> void;

    private:
        Log& m_Log;
        LifecycleStore& m_LifecycleStore;
        MemoryReaderService& m_MemoryReaderService;
        CaptureStore& m_CaptureStore;
        TelemetryStore& m_TelemetryStore;

        // Snapshots Capture reuses (D276). Owned and changed only by Capture, on the AI thread: Cleanup, which runs on the game thread,
        // only drops the published Snapshot of the store and never touches the pool. A new teardown epoch starts a new pool.
        static constexpr std::size_t k_SnapshotPoolCapacity{ 4 };
        std::vector<std::shared_ptr<Snapshot>> m_Pool{};
        std::uint64_t m_PoolEpoch{ std::numeric_limits<std::uint64_t>::max() };

        // return: A snapshot to fill: one from the pool that nobody else holds (emptied of objects), else a new one that joins the pool
        // while it has room (above k_SnapshotPoolCapacity it is not kept: the dump queue or a viewer may hold many).
        // note: Runs on the AI thread inside the read window.
        auto AcquireSnapshot(std::uint64_t epoch) -> std::shared_ptr<Snapshot>;

        // Teardown epoch the capacities were checked for, or the maximum before the first check. A new load runs under a new epoch, so
        // nothing resets it: not the game thread (Cleanup) and not a tick that was still running after the teardown began, which marks
        // the epoch it started under (read before the teardown), never the next one.
        std::atomic<std::uint64_t> m_CheckedEpoch{ std::numeric_limits<std::uint64_t>::max() };

        // Compares the capacity of the object and player tables of the engine (data array header) with k_MaxObjects and k_MaxPlayers,
        // once per map load. A difference is a WARNING with both values, nothing else changes.
        // note: Runs on the AI thread inside the read window.
        auto CheckCapacities(std::uintptr_t threadContext, std::uint64_t epoch) -> void;

        // Copies the datum of every live object of the object table: the slots below the first-unused mark of the engine data array
        // (every live object is below it), at most the capacity of the array and k_MaxObjects.
        // note: The mark is read on every call. Without a readable mark or capacity it walks what it can: the capacity, then k_MaxObjects.
        // It also tells the telemetry the mark and the live count (the maxima of a run).
        auto CopyObjects(Snapshot& snapshot, std::uintptr_t threadContext, std::uintptr_t tableBase) -> void;

        // Copies RegionSize(id) bytes from the address into the region.
        // note: An address of 0 or a failed read leave the region as not captured.
        auto CopyRegion(Snapshot& snapshot, RegionId id, std::uintptr_t address) -> void;

        // return: Address of the placement array of the map variant, or 0 outside multiplayer games or if a pointer is missing.
        // note: The map variant changes with every game, so it is resolved from the thread context on every call.
        auto FindPlacements(std::uintptr_t threadContext) -> std::uintptr_t;
    };
}