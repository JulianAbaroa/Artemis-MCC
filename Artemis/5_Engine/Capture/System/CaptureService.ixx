export module Engine.Capture.System;

import Common.Log.System;
import Services.Telemetry.State;
import Engine.Base.Action.Type;
import Engine.Base.Lifecycle.State;
import Engine.Base.Memory.System;
import Engine.Capture.Type;
import Engine.Capture.State;
import Blam.Layout.Snapshot.Type;
import std;

namespace
{
    using Engine::Capture::Type::k_TickQueueDepth;
}

export namespace Engine::Capture::System
{
    // Copies everything the tick pipeline reads from the game into a snapshot: the live objects and the regions of memory.
    class CaptureService
    {
    private:
        using Log = Common::Log::System::Log;

        using TelemetryStore = Services::Telemetry::State::TelemetryStore;

        using AppliedActionsPort = Engine::Base::Action::Type::AppliedActionsPort;

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
        // note: Game thread only, between two ticks (D305): the engine writes nothing then, so the copy is stable. The AI thread works on
        // the returned Snapshot and drops it when it is done.
        auto Capture() -> std::shared_ptr<const Snapshot>;

        // Copies only what the flag pass needs into a light Snapshot (IsLight): the objects of kind k_LightKindId (the flags are weapons,
        // and this project cannot filter by tag) and the regions IsLightRegion says (game clock, game engine globals and megalo objects).
        // return: The snapshot, not published: the store keeps the last full one. Without the thread context it holds nothing.
        // note: Game thread only, like Capture. It takes no telemetry and never checks the capacities.
        auto CaptureLight() -> std::shared_ptr<const Snapshot>;

        // Connects the owner of the applied actions, which Capture copies into the region PlayerActions (D371). Null disconnects it.
        // note: Without one the region stays empty (no error, no log). Connect it before the tick hooks go in, disconnect it after they are out.
        auto SetAppliedActionsPort(AppliedActionsPort* port) -> void;

        // Clears the stored addresses and snapshot.
        auto Cleanup() -> void;

    private:
        Log& m_Log;
        LifecycleStore& m_LifecycleStore;
        MemoryReaderService& m_MemoryReaderService;
        CaptureStore& m_CaptureStore;
        TelemetryStore& m_TelemetryStore;
        std::atomic<AppliedActionsPort*> m_AppliedActionsPort{ nullptr };
        // Sequence of the actions the last full capture copied, 0 when none. Game thread only.
        std::uint64_t m_LastAppliedSequence{ 0 };

        // Snapshots Capture reuses (D276). The pool is filled and changed only by Capture, on the game thread (D305), and the AI thread
        // releases a Snapshot by dropping its reference: the pool takes one again when only it holds it. A new teardown epoch starts a new
        // pool.
        // note: Capacity = the queue (k_TickQueueDepth) + the one the AI thread processes + the one being filled + the published one of the
        // store + one an observer keeps (the viewer). A Snapshot above it is not kept: the dump queue or a viewer may hold many.
        static constexpr std::size_t k_SnapshotPoolCapacity{ k_TickQueueDepth + 4 };
        std::vector<std::shared_ptr<Snapshot>> m_Pool{};
        std::uint64_t m_PoolEpoch{ std::numeric_limits<std::uint64_t>::max() };

        // return: A snapshot to fill: one from the pool that nobody else holds (emptied of objects), else a new one that joins the pool
        // while it has room (above k_SnapshotPoolCapacity it is not kept: the dump queue or a viewer may hold many).
        // note: Game thread only.
        auto AcquireSnapshot(std::uint64_t epoch) -> std::shared_ptr<Snapshot>;

        // Teardown epoch the capacities were checked for, or the maximum before the first check. A new load runs under a new epoch, so
        // nothing resets it: not the game thread (Cleanup) and not a tick that was still running after the teardown began, which marks
        // the epoch it started under (read before the teardown), never the next one.
        std::atomic<std::uint64_t> m_CheckedEpoch{ std::numeric_limits<std::uint64_t>::max() };

        // Compares the capacity of the object and player tables of the engine (data array header) with k_MaxObjects and k_MaxPlayers,
        // once per map load. A difference is a WARNING with both values, nothing else changes.
        // note: Game thread only.
        auto CheckCapacities(std::uintptr_t threadContext, std::uint64_t epoch) -> void;

        // Copies the datum of every live object of the object table: the slots below the first-unused mark of the engine data array
        // (every live object is below it), at most the capacity of the array and k_MaxObjects.
        // note: The mark is read on every call. Without a readable mark or capacity it walks what it can: the capacity, then k_MaxObjects.
        // It also tells the telemetry the mark and the live count (the maxima of a run).
        // param isLight: Copies only the objects of kind k_LightKindId and leaves the telemetry alone.
        auto CopyObjects(Snapshot& snapshot, std::uintptr_t threadContext, std::uintptr_t tableBase, bool isLight) -> void;

        // Copies RegionSize(id) bytes from the address into the region.
        // note: An address of 0 or a failed read leave the region as not captured.
        auto CopyRegion(Snapshot& snapshot, RegionId id, std::uintptr_t address) -> void;

        // Lays out the actions the simulation applied in the tick that just completed as the region PlayerActions. The region is empty
        // when there is no port, nothing was published yet, or the Sequence did not advance since the previous full capture (a tick that
        // published nothing, as the end of a match: the actions would be a stale repeat).
        // note: Game thread only. It is not memory of the game: the port gives it.
        auto CopyAppliedActions(Snapshot& snapshot) -> void;

        // return: Address of the placement array of the map variant, or 0 outside multiplayer games or if a pointer is missing.
        // note: The map variant changes with every game, so it is resolved from the thread context on every call.
        auto FindPlacements(std::uintptr_t threadContext) -> std::uintptr_t;
    };
}