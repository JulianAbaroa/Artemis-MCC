export module Tables.Interaction.Hook;

import Common.Log.System;
import Platform.Memory.System;
import Tables.Interaction.State;
import std;

export namespace Tables::Interaction::Hook
{
    // Finds the engine interaction tables (one per view) and stores the address of the first one.
    class InteractionTableLocator
    {
    private:
        using Log = Common::Log::System::Log;

        using AOBService = Platform::Memory::System::AOBService;

        using InteractionStore = Tables::Interaction::State::InteractionStore;

    public:
        InteractionTableLocator(Log& log, AOBService& aobService,
            InteractionStore& interactionStore) : m_Log(log),
            m_AOBService(aobService), m_InteractionStore(interactionStore) {}
        ~InteractionTableLocator() = default;

        // Stores the table base unless it is already stored.
        // note: Logs an error and stores nothing if the table is not found. Call it again later.
        auto FindAndStoreTableBase() -> void;

    private:
        Log& m_Log;
        AOBService& m_AOBService;
        InteractionStore& m_InteractionStore;

        // return: Address of the first output user entry, or 0 if the thread context is not ready.
        auto GetInteractionTable() -> std::uintptr_t;
    };
}