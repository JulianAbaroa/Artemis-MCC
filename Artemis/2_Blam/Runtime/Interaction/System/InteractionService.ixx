export module Blam.Runtime.Interaction.System;

import Common.Log.System;
import Blam.Layout.Snapshot.Type;
import Blam.Runtime.Interaction.Type;
import Blam.Runtime.Interaction.State;
import std;

export namespace Blam::Runtime::Interaction::System
{
    // Reads the engine interaction tables and publishes the interaction of every local player to the store.
    class InteractionService
    {
    private:
        using Log = Common::Log::System::Log;

        using AliveInteraction = Blam::Runtime::Interaction::Type::Alive::AliveInteraction;
        using InteractionStore = Blam::Runtime::Interaction::State::InteractionStore;

        using Snapshot = Blam::Layout::Snapshot::Type::Snapshot;

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