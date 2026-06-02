#include "pch.h"

#include "State_PlayerGraph.h"

bool State_PlayerGraph::HasChanged() const
{
    return m_HasChanged.load();
}

const std::vector<PlayerTree>& State_PlayerGraph::GetTrees() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Trees;
}

void State_PlayerGraph::SetTrees(std::vector<PlayerTree>&& trees)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Trees = std::move(trees);
    m_HasChanged.store(true);
}

void State_PlayerGraph::Cleanup()
{
    m_HasChanged.store(false);

    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Trees.clear();
}