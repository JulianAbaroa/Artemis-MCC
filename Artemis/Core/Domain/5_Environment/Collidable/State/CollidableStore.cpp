module Environment.Collidable.State;

namespace Environment::Collidable::State
{
    auto CollidableStore::Publish(Collidables instances) -> void
    {
        auto snap = std::make_shared<const Collidables>(std::move(instances));
        m_pCollidables.store(snap, std::memory_order_release);
    }

    auto CollidableStore::Acquire() const -> std::shared_ptr<const Collidables>
    {
        return m_pCollidables.load(std::memory_order_acquire);
    }

    auto CollidableStore::Cleanup() -> void
    {
        m_pCollidables.store(nullptr, std::memory_order_release);
    }
}