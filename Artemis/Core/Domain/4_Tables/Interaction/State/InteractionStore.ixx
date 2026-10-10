export module Tables.Interaction.State;

import Tables.Interaction.Type;
import std;

export namespace Tables::Interaction::State
{
    // Holds the base address of the engine interaction table and the last published interaction.
    // note: Lock-free. A published interaction is never modified.
    class InteractionStore
    {
    private:
        using AliveInteraction = Tables::Interaction::Type::Alive::AliveInteraction;

    public:
        InteractionStore() = default;
        ~InteractionStore() = default;

        // return: Address of the interaction table, or 0 if it was not found yet.
        auto GetBase() const -> std::uintptr_t;
        auto SetBase(std::uintptr_t pointer) -> void;

        // Replaces the interaction that Acquire returns.
        auto Publish(AliveInteraction interaction) -> void;

        // return: Last published interaction, or null before the first one and after Cleanup.
        auto Acquire() const -> std::shared_ptr<const AliveInteraction>;

        // Clears the base address and drops the published interaction.
        auto Cleanup() -> void;

    private:
        std::atomic<std::uintptr_t> m_InteractionTableBase{ 0 };
        std::atomic<std::shared_ptr<const AliveInteraction>> m_AliveInteraction{};
    };
}