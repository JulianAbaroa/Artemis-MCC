#include "pch.h"

#include "State_Affordances.h"

void State_Affordances::Publish(Affordances interactables)
{
    auto snap = std::make_shared<const Affordances>(std::move(interactables));
    m_pAffordances.store(snap, std::memory_order_release);
}

std::shared_ptr<const Affordances> State_Affordances::Acquire() const
{
    return m_pAffordances.load(std::memory_order_acquire);
}

void State_Affordances::Cleanup()
{
	m_pAffordances.store(nullptr, std::memory_order_release);
}