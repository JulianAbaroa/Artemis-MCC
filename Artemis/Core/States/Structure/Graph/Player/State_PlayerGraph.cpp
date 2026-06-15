#include "pch.h"

#include "State_PlayerGraph.h"

void State_PlayerGraph::Publish(PlayerGraph trees)
{
    auto snap = std::make_shared<const PlayerGraph>(std::move(trees));
    m_pPlayerGraph.store(snap, std::memory_order_release);
}

std::shared_ptr<const PlayerGraph> State_PlayerGraph::Acquire() const
{
    return m_pPlayerGraph.load(std::memory_order_acquire);
}

void State_PlayerGraph::Cleanup()
{
    m_pPlayerGraph.store(nullptr, std::memory_order_release);
}