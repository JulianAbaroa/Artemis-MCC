export module Tables.Interaction.Hook;

import Service.Logs.System;
import Platform.Memory.System;
import Tables.Interaction.State;
import std;

export namespace Tables::Interaction::Hook
{
    // Finds the engine interaction table and stores its base address.
    class InteractionTableLocator
    {
    private:
        using LogsService = Service::Logs::System::LogsService;

        using AOBService = Platform::Memory::System::AOBService;

        using InteractionStore = Tables::Interaction::State::InteractionStore;

    public:
        InteractionTableLocator(LogsService& logsService, AOBService& aobService,
            InteractionStore& interactionStore) : m_LogsService(logsService),
            m_AOBService(aobService), m_InteractionStore(interactionStore) {}
        ~InteractionTableLocator() = default;

        // Stores the table base unless it is already stored.
        // note: Logs an error and stores nothing if the table is not found. Call it again later.
        auto FindAndStoreTableBase() -> void;

    private:
        LogsService& m_LogsService;
        AOBService& m_AOBService;
        InteractionStore& m_InteractionStore;

        // return: Address of the interaction table, or 0 if the thread context is not ready.
        auto GetInteractionTable() -> std::uintptr_t;
    };
}