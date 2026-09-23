module Environment.Fixtures.State;

namespace Environment::Fixtures::State
{
    auto FixturesStore::Publish(Fixtures data) -> void
    {
        auto snap = std::make_shared<const Fixtures>(std::move(data));
        m_pFixtures.store(snap, std::memory_order_release);
    }

    auto FixturesStore::Acquire() const -> std::shared_ptr<const Fixtures>
    {
        return m_pFixtures.load(std::memory_order_acquire);
    }

    auto FixturesStore::Cleanup() -> void
    {
        m_pFixtures.store(nullptr, std::memory_order_release);
    }
}