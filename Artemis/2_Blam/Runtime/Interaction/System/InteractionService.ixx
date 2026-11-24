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
        using ObjectSelection = Blam::Runtime::Interaction::Type::Alive::ObjectSelection;
        using InteractionStore = Blam::Runtime::Interaction::State::InteractionStore;

        using RegionId = Blam::Layout::Snapshot::Type::RegionId;
        using Snapshot = Blam::Layout::Snapshot::Type::Snapshot;

    public:
        InteractionService(Log& log, InteractionStore& interactionStore) :
            m_Log(log), m_InteractionStore(interactionStore) {}
        ~InteractionService() = default;

        // Reads the table of every view from the snapshot and publishes it under the local player the view shows.
        // note: Publishes empty interactions while the interaction tables or the player mapping are not captured.
        // A local player without a view keeps an empty interaction.
        auto UpdateInteractionTable(const Snapshot& snapshot) -> void;

        // Decodes the first interaction record (+0x54) of the action the simulation applied to each player with a bit in the mask of the
        // region PlayerActions, and publishes the selections by player table index (D371).
        // note: Publishes all None while the region is not captured (no port, a tick without published actions, a recording made before
        // it existed). A player without an action keeps None.
        auto UpdateObjectSelections(const Snapshot& snapshot) -> void;

        // Clears the stored interactions.
        auto Cleanup() -> void;

    private:
        Log& m_Log;
        InteractionStore& m_InteractionStore;

        // return: The kind, detail and object of the record that starts at the offset of the region. The one decoder of the first record,
        // for the table of a local player and for an applied action.
        static auto ReadSelection(const Snapshot& snapshot, RegionId region, std::size_t recordOffset) -> ObjectSelection;

        // Reads the interaction that starts at the offset of the interaction region.
        auto ReadInteraction(const Snapshot& snapshot, std::size_t interactionOffset) -> AliveInteraction;
    };
}