#include "pch.h"

#include "State_Collidables.h"

void State_Collidables::Publish(Collidables instances)
{
    auto snap = std::make_shared<const Collidables>(std::move(instances));
    m_pCollidables.store(snap, std::memory_order_release);
}

std::shared_ptr<const Collidables> State_Collidables::Acquire() const
{
    return m_pCollidables.load(std::memory_order_acquire);
}

void State_Collidables::Cleanup()
{
    m_pCollidables.store(nullptr, std::memory_order_release);
}