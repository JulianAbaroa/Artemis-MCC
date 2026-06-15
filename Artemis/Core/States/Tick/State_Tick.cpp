#include "pch.h"

#include "State_Tick.h"

void State_Tick::Publish(Tick tick)
{
    auto snap = std::make_shared<const Tick>(std::move(tick));
    m_pTick.store(snap, std::memory_order_release);
}

std::shared_ptr<const Tick> State_Tick::Acquire() const
{
    return m_pTick.load(std::memory_order_acquire);
}

void State_Tick::Cleanup()
{
    m_pTick.store(nullptr, std::memory_order_release);
}