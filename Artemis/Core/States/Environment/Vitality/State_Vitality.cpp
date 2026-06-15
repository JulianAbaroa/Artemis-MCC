#include "pch.h"

#include "State_Vitality.h"

void State_Vitality::Publish(Vitalities vitalities)
{
    auto snap = std::make_shared<const Vitalities>(std::move(vitalities));
    m_pVitalities.store(snap, std::memory_order_release);
}

std::shared_ptr<const Vitalities> State_Vitality::Acquire() const
{
    return m_pVitalities.load(std::memory_order_acquire);
}

void State_Vitality::Cleanup()
{
    m_pVitalities.store(nullptr, std::memory_order_release);
}