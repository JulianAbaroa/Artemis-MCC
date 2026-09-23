module Egocentric.Self.State;

namespace Egocentric::Self::State
{
    auto SelfStore::Publish(Self self) -> void
    {
        auto snap = std::make_shared<const Self>(std::move(self));
        m_pSelf.store(snap, std::memory_order_release);
    }

    auto SelfStore::Acquire() const -> std::shared_ptr<const Self>
    {
        return m_pSelf.load(std::memory_order_acquire);
    }

    auto SelfStore::Cleanup() -> void
    {
        m_pSelf.store(nullptr, std::memory_order_release);
    }
}