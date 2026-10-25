export module Tables.Shadow.System:Interaction;

import Common.Log.System;
import Platform.Memory.System;
import Platform.Capture.State;
import Tables.Interaction.Type;
import Tables.Interaction.State;
import std;

export namespace Tables::Shadow::System
{
    // Old live read of the interactions, kept to compare it with the one that reads the snapshot (M4a, D188).
    // note: Temporary copy of the InteractionService of before the snapshot. M4b deletes it.
    class ShadowInteractionService
    {
    private:
        using Log = Common::Log::System::Log;

        using MemoryReaderService = Platform::Memory::System::MemoryReaderService;

        using CaptureStore = Platform::Capture::State::CaptureStore;

        using AliveInteraction = Tables::Interaction::Type::Alive::AliveInteraction;
        using InteractionStore = Tables::Interaction::State::InteractionStore;

    public:
        ShadowInteractionService(Log& log,
            MemoryReaderService& memoryReaderService, CaptureStore& captureStore,
            InteractionStore& interactionStore) :
            m_Log(log), m_MemoryReaderService(memoryReaderService),
            m_CaptureStore(captureStore), m_InteractionStore(interactionStore) {}
        ~ShadowInteractionService() = default;

        // Reads the table of every view and publishes it under the local player the view shows.
        // note: Publishes empty interactions while the table base or the player mapping is not found.
        // A local player without a view keeps an empty interaction.
        auto UpdateInteractionTable() -> void;

        // Clears the stored table base and interactions.
        auto Cleanup() -> void;

    private:
        Log& m_Log;
        MemoryReaderService& m_MemoryReaderService;
        CaptureStore& m_CaptureStore;
        InteractionStore& m_InteractionStore;

        // Reads the interaction that starts at the address.
        auto ReadInteraction(std::uintptr_t interactionBase) -> AliveInteraction;
    };
}