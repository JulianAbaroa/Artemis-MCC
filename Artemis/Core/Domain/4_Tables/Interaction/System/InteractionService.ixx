export module Tables.Interaction.System;

import Common.Log.System;
import Platform.Memory.System;
import Tables.Player.State;
import Tables.Interaction.Type;
import Tables.Interaction.State;
import std;

export namespace Tables::Interaction::System
{
    // Reads the engine interaction tables and publishes the interaction of every local player to the store.
    class InteractionService
    {
    private:
        using Log = Common::Log::System::Log;

        using MemoryReaderService = Platform::Memory::System::MemoryReaderService;

        using PlayerTableStore = Tables::Player::State::PlayerTableStore;

        using AliveInteraction = Tables::Interaction::Type::Alive::AliveInteraction;
        using InteractionStore = Tables::Interaction::State::InteractionStore;

    public:
        InteractionService(Log& log,
            MemoryReaderService& memoryReaderService, PlayerTableStore& playerStore,
            InteractionStore& interactionStore) :
            m_Log(log), m_MemoryReaderService(memoryReaderService),
            m_PlayerStore(playerStore), m_InteractionStore(interactionStore) {}
        ~InteractionService() = default;

        // Reads the table of every view and publishes it under the local player the view shows.
        // note: Publishes empty interactions while the table base or the player mapping is not found.
        // A local player without a view keeps an empty interaction.
        auto UpdateInteractionTable() -> void;

        // Clears the stored table base and interactions.
        auto Cleanup() -> void;

    private:
        Log& m_Log;
        MemoryReaderService& m_MemoryReaderService;
        PlayerTableStore& m_PlayerStore;
        InteractionStore& m_InteractionStore;

        // Reads the interaction that starts at the address.
        auto ReadInteraction(std::uintptr_t interactionBase) -> AliveInteraction;
    };
}