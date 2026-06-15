#include "pch.h"

#include "State_Fixtures.h"

void State_Fixtures::Publish(Fixtures data)
{
    auto snap = std::make_shared<const Fixtures>(std::move(data));
    m_pFixtures.store(snap, std::memory_order_release);
}

std::shared_ptr<const Fixtures> State_Fixtures::Acquire() const
{
    return m_pFixtures.load(std::memory_order_acquire);
}

void State_Fixtures::Cleanup()
{
    m_pFixtures.store(nullptr, std::memory_order_release);
}