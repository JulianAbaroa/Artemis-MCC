module Tables.Interaction.State;

namespace Tables::Interaction::State
{
    auto InteractionStore::GetBase() const -> std::uintptr_t
    {
        return m_InteractionTableBase.load();
    }

    auto InteractionStore::SetBase(std::uintptr_t pointer) -> void
    {
        m_InteractionTableBase.store(pointer);
    }

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
        m_InteractionTableBase.store(0);
        m_LocalInteractions.store(nullptr, std::memory_order_release);
    }
}