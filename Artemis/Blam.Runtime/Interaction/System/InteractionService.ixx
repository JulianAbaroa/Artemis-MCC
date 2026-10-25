export module Tables.Interaction.System;

import Common.Log.System;
import Layout.Snapshot.Type;
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

        using AliveInteraction = Tables::Interaction::Type::Alive::AliveInteraction;
        using InteractionStore = Tables::Interaction::State::InteractionStore;

        using Snapshot = Layout::Snapshot::Type::Snapshot;

    public:
        InteractionService(Log& log, InteractionStore& interactionStore) :
            m_Log(log), m_InteractionStore(interactionStore) {}
        ~InteractionService() = default;

        // Reads the table of every view from the snapshot and publishes it under the local player the view shows.
        // note: Publishes empty interactions while the interaction tables or the player mapping are not captured.
        // A local player without a view keeps an empty interaction.
        auto UpdateInteractionTable(const Snapshot& snapshot) -> void;

        // Clears the stored interactions.
        auto Cleanup() -> void;

    private:
        Log& m_Log;
        InteractionStore& m_InteractionStore;

        // Reads the interaction that starts at the offset of the interaction region.
        auto ReadInteraction(const Snapshot& snapshot, std::size_t interactionOffset) -> AliveInteraction;
    };
}