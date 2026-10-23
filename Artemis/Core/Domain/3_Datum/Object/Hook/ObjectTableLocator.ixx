export module Datum.Object.Hook;

import Common.Log.System;
import Platform.Memory.System;
import Datum.Object.State;
import std;

export namespace Datum::Object::Hook
{
    // Finds the engine thread context and the object table, and stores both.
    class ObjectTableLocator
    {
    private:
        using Log = Common::Log::System::Log;

        using AOBService = Platform::Memory::System::AOBService;

        using DatumStore = Datum::Object::State::DatumStore;

    public:
        ObjectTableLocator(Log& log, AOBService& aobService,
            DatumStore& datumStore) : m_Log(log),
            m_AOBService(aobService), m_DatumStore(datumStore) {}
        ~ObjectTableLocator() = default;

        // Stores the thread context and the table base unless they are already stored.
        // note: Logs an error and stores nothing if the table is not found. Call it again later.
        auto FindAndStoreTableBase() -> void;

    private:
        Log& m_Log;
        AOBService& m_AOBService;
        DatumStore& m_DatumStore;

        // return: Engine thread context of the calling thread, or 0 if it is not ready.
        // note: Read from the TLS slot of the engine, so it must run on the game thread.
        auto GetThreadContext() -> std::uintptr_t;

        // return: Address of the object table, or 0 if the thread context is not ready.
        auto GetObjectTable(std::uintptr_t threadContext) -> std::uintptr_t;
    };
}