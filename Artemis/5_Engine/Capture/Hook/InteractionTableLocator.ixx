export module Engine.Capture.Hook:InteractionTable;

import Common.Log.System;
import Engine.Base.Memory.System;
import Engine.Capture.State;
import std;

export namespace Engine::Capture::Hook
{
    // Finds the engine interaction tables (one per view) and stores the address of the first one.
    class InteractionTableLocator
    {
    private:
        using Log = Common::Log::System::Log;

        using AOBService = Engine::Base::Memory::System::AOBService;

        using CaptureStore = Engine::Capture::State::CaptureStore;

    public:
        InteractionTableLocator(Log& log, AOBService& aobService,
            CaptureStore& captureStore) : m_Log(log),
            m_AOBService(aobService), m_CaptureStore(captureStore) {}
        ~InteractionTableLocator() = default;

        // Stores the table base unless it is already stored.
        // note: Logs an error and stores nothing if the table is not found. Call it again later.
        auto FindAndStoreTableBase() -> void;

    private:
        Log& m_Log;
        AOBService& m_AOBService;
        CaptureStore& m_CaptureStore;

        // return: Address of the first output user entry, or 0 if the thread context is not ready.
        auto GetInteractionTable() -> std::uintptr_t;
    };
}