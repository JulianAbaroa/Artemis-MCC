#include "pch.h"

#include "State_Self.h"

void State_Self::Publish(Self self)
{
    auto snap = std::make_shared<const Self>(std::move(self));
    m_pSelf.store(snap, std::memory_order_release);
}

std::shared_ptr<const Self> State_Self::Acquire() const
{
    return m_pSelf.load(std::memory_order_acquire);
}

void State_Self::Cleanup()
{
    m_pSelf.store(nullptr, std::memory_order_release);
}