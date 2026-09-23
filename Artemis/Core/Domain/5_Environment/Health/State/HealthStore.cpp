module Environment.Health.State;

namespace Environment::Health::State
{
    auto HealthStore::Publish(Vitalities vitalities) -> void
    {
        auto snap = std::make_shared<const Vitalities>(std::move(vitalities));
        m_pVitalities.store(snap, std::memory_order_release);
    }

    auto HealthStore::Acquire() const -> std::shared_ptr<const Vitalities>
    {
        return m_pVitalities.load(std::memory_order_acquire);
    }

    auto HealthStore::Cleanup() -> void
    {
        m_pVitalities.store(nullptr, std::memory_order_release);
    }
}