export module Platform.Capture.System;

import Common.Log.System;
import Platform.Memory.System;
import Platform.Capture.State;
import Layout.Snapshot.Type;
import std;

export namespace Platform::Capture::System
{
    // Copies everything the tick pipeline reads from the game into a snapshot: the live objects and the regions of memory.
    class CaptureService
    {
    private:
        using Log = Common::Log::System::Log;

        using MemoryReaderService = Platform::Memory::System::MemoryReaderService;

        using CaptureStore = Platform::Capture::State::CaptureStore;

        using RegionId = Layout::Snapshot::Type::RegionId;
        using Snapshot = Layout::Snapshot::Type::Snapshot;

    public:
        CaptureService(Log& log,
            MemoryReaderService& memoryReaderService,
            CaptureStore& captureStore) :
            m_Log(log), m_MemoryReaderService(memoryReaderService),
            m_CaptureStore(captureStore) {}
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
        MemoryReaderService& m_MemoryReaderService;
        CaptureStore& m_CaptureStore;

        // Copies the datum of every live object of the object table.
        auto CopyObjects(Snapshot& snapshot, std::uintptr_t tableBase) -> void;

        // Copies RegionSize(id) bytes from the address into the region.
        // note: An address of 0 or a failed read leave the region as not captured.
        auto CopyRegion(Snapshot& snapshot, RegionId id, std::uintptr_t address) -> void;

        // return: Address of the placement array of the map variant, or 0 outside multiplayer games or if a pointer is missing.
        // note: The map variant changes with every game, so it is resolved from the thread context on every call.
        auto FindPlacements(std::uintptr_t threadContext) -> std::uintptr_t;
    };
}