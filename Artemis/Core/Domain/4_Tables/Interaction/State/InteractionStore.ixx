export module Tables.Interaction.State;

import Tables.Interaction.Type;
import std;

export namespace Tables::Interaction::State
{
    // Holds the base address of the engine interaction tables and the last published interactions.
    // note: Lock-free. Published interactions are never modified.
    class InteractionStore
    {
    private:
        using LocalInteractions = Tables::Interaction::Type::Alive::LocalInteractions;

    public:
        InteractionStore() = default;
        ~InteractionStore() = default;

        // return: Address of the first output user entry, or 0 if it was not found yet.
        auto GetBase() const -> std::uintptr_t;
        auto SetBase(std::uintptr_t pointer) -> void;

        // Replaces the interactions that Acquire returns.
        auto Publish(LocalInteractions interactions) -> void;

        // return: Last published interactions, or null before the first ones and after Cleanup.
        auto Acquire() const -> std::shared_ptr<const LocalInteractions>;

        // Clears the base address and drops the published interactions.
        auto Cleanup() -> void;

    private:
        std::atomic<std::uintptr_t> m_InteractionTableBase{ 0 };
        std::atomic<std::shared_ptr<const LocalInteractions>> m_LocalInteractions{};
    };
}