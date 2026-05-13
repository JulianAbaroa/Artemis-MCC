#include "pch.h"

// Header.
#include "State_PlayerGraph.h"

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

bool State_PlayerGraph::HasChanged() const
{
    return m_HasChanged.load();
}

void State_PlayerGraph::ClearChanged()
{
    m_HasChanged.store(false);
}

void State_PlayerGraph::Cleanup()
{
    m_HasChanged.store(false);

    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Trees.clear();
}