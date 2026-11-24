export module Blam.Runtime.Interaction.State;

import Blam.Runtime.Interaction.Type;
import std;

export namespace Blam::Runtime::Interaction::State
{
    // Holds the last published interactions and the object selections of the players.
    // note: Lock-free. Published interactions are never modified.
    class InteractionStore
    {
    private:
        using LocalInteractions = Blam::Runtime::Interaction::Type::Alive::LocalInteractions;
        using ObjectSelections = Blam::Runtime::Interaction::Type::Alive::ObjectSelections;

    public:
        InteractionStore() = default;
        ~InteractionStore() = default;

        // Replaces the interactions that Acquire returns.
        auto Publish(LocalInteractions interactions) -> void;

        // return: Last published interactions, or null before the first ones and after Cleanup.
        auto Acquire() const -> std::shared_ptr<const LocalInteractions>;

        // Replaces the selections that AcquireSelections returns.
        auto PublishSelections(ObjectSelections selections) -> void;

        // return: Last published selections of the players with an applied action, or null before the first ones and after Cleanup.
        auto AcquireSelections() const -> std::shared_ptr<const ObjectSelections>;

        // Drops the published interactions and selections.
        auto Cleanup() -> void;

    private:
        std::atomic<std::shared_ptr<const LocalInteractions>> m_LocalInteractions{};
        std::atomic<std::shared_ptr<const ObjectSelections>> m_Selections{};
    };
}