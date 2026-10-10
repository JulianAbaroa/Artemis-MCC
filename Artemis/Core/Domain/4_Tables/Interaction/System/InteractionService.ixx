export module Tables.Interaction.System;

import Service.Logs.System;
import Platform.Memory.System;
import Tables.Interaction.State;

export namespace Tables::Interaction::System
{
    // Reads the engine interaction table and publishes it to the store.
    class InteractionService
    {
    private:
        using LogsService = Service::Logs::System::LogsService;

        using MemoryReaderService = Platform::Memory::System::MemoryReaderService;

        using InteractionStore = Tables::Interaction::State::InteractionStore;

    public:
        InteractionService(LogsService& logsService,
            MemoryReaderService& memoryReaderService,
            InteractionStore& interactionStore) :
            m_LogsService(logsService), m_MemoryReaderService(memoryReaderService),
            m_InteractionStore(interactionStore) {}
        ~InteractionService() = default;

        // Reads the interaction table and publishes it as the new interaction.
        // note: Publishes an empty interaction while the table base is not found.
        auto UpdateInteractionTable() -> void;

        // Clears the stored table base and interaction.
        auto Cleanup() -> void;

    private:
        LogsService& m_LogsService;
        MemoryReaderService& m_MemoryReaderService;
        InteractionStore& m_InteractionStore;
    };
}