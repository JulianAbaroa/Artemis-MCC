export module Datum.Object.System;

import Common.Log.System;
import Platform.Memory.System;
import Datum.Object.State;
import std;

export namespace Datum::Object::System
{
    // Copies the datum of every live object of the engine object table, and the placements of the map variant, and publishes the copies to the store.
    class DatumService
    {
    private:
        using Log = Common::Log::System::Log;

        using MemoryReaderService = Platform::Memory::System::MemoryReaderService;

        using DatumStore = Datum::Object::State::DatumStore;

    public:
        DatumService(Log& log,
            MemoryReaderService& memoryReaderService,
            DatumStore& datumStore) :
            m_Log(log), m_MemoryReaderService(memoryReaderService),
            m_DatumStore(datumStore) {}
        ~DatumService() = default;

        // Reads the object table and the map variant and publishes a snapshot with the datum of each live object and the placements.
        // note: Publishes nothing while the table base is not found.
        auto UpdateObjectTable() -> void;

        // Clears the stored table base and snapshot.
        auto Cleanup() -> void;

    private:
        Log& m_Log;
        MemoryReaderService& m_MemoryReaderService;
        DatumStore& m_DatumStore;

        // Copies the placement array of the map variant.
        // return: The bytes, or empty outside multiplayer games, before the thread context is found or if the read fails.
        // note: The map variant changes with every game, so it is resolved from the thread context on every call.
        auto ReadPlacements() -> std::vector<std::uint8_t>;
    };
}