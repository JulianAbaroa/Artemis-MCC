module Export.Tick.State;

namespace Export::Tick::State
{
    auto TickStore::Publish(Tick tick) -> void
    {
        auto snap = std::make_shared<const Tick>(std::move(tick));
        m_pTick.store(snap, std::memory_order_release);
    }

    auto TickStore::Acquire() const -> std::shared_ptr<const Tick>
    {
        return m_pTick.load(std::memory_order_acquire);
    }

    auto TickStore::Cleanup() -> void
    {
        m_pTick.store(nullptr, std::memory_order_release);
    }
}