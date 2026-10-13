export module Datum.Object.System;

import Service.Logs.System;
import Platform.Memory.System;
import Datum.Object.State;
import std;

export namespace Datum::Object::System
{
    // Copies the datum of every live object of the engine object table and publishes the copies to the store.
    class DatumService
    {
    private:
        using LogsService = Service::Logs::System::LogsService;

        using MemoryReaderService = Platform::Memory::System::MemoryReaderService;

        using DatumStore = Datum::Object::State::DatumStore;

    public:
        DatumService(LogsService& logsService,
            MemoryReaderService& memoryReaderService,
            DatumStore& datumStore) :
            m_LogsService(logsService), m_MemoryReaderService(memoryReaderService),
            m_DatumStore(datumStore) {}
        ~DatumService() = default;

        // Reads the object table and publishes a snapshot with the datum of each live object.
        // note: Publishes nothing while the table base is not found.
        auto UpdateObjectTable() -> void;

        // Clears the stored table base and snapshot.
        auto Cleanup() -> void;

    private:
        LogsService& m_LogsService;
        MemoryReaderService& m_MemoryReaderService;
        DatumStore& m_DatumStore;
    };
}