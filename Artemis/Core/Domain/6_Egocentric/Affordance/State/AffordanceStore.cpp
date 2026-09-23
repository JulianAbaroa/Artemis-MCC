module Egocentric.Affordance.State;

namespace Egocentric::Affordance::State
{
    auto AffordanceStore::Publish(Affordances interactables) -> void
    {
        auto snap = std::make_shared<const Affordances>(std::move(interactables));
        m_pAffordances.store(snap, std::memory_order_release);
    }

    auto AffordanceStore::Acquire() const -> std::shared_ptr<const Affordances>
    {
        return m_pAffordances.load(std::memory_order_acquire);
    }

    auto AffordanceStore::Cleanup() -> void
    {
        m_pAffordances.store(nullptr, std::memory_order_release);
    }
}