export module Blam.Runtime.Interaction.State;

import Blam.Runtime.Interaction.Type;
import std;

export namespace Blam::Runtime::Interaction::State
{
    // Holds the last published interactions.
    // note: Lock-free. Published interactions are never modified.
    class InteractionStore
    {
    private:
        using LocalInteractions = Blam::Runtime::Interaction::Type::Alive::LocalInteractions;

    public:
        InteractionStore() = default;
        ~InteractionStore() = default;

        // Replaces the interactions that Acquire returns.
        auto Publish(LocalInteractions interactions) -> void;

        // return: Last published interactions, or null before the first ones and after Cleanup.
        auto Acquire() const -> std::shared_ptr<const LocalInteractions>;

        // Drops the published interactions.
        auto Cleanup() -> void;

    private:
        std::atomic<std::shared_ptr<const LocalInteractions>> m_LocalInteractions{};
    };
}