module Tables.Interaction.State;

namespace Tables::Interaction::State
{
    auto InteractionStore::Publish(LocalInteractions interactions) -> void
    {
        auto snapshot = std::make_shared<const LocalInteractions>(std::move(interactions));
        m_LocalInteractions.store(snapshot, std::memory_order_release);
    }

    auto InteractionStore::Acquire() const -> std::shared_ptr<const LocalInteractions>
    {
        return m_LocalInteractions.load(std::memory_order_acquire);
    }

    auto InteractionStore::Cleanup() -> void
    {
        m_LocalInteractions.store(nullptr, std::memory_order_release);
    }
}