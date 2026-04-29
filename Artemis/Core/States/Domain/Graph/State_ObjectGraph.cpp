#include "pch.h"
#include "Core/States/Domain/Graph/State_ObjectGraph.h"

const std::unordered_map<uint32_t, ObjectNode>& State_ObjectGraph::GetNodes() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_Nodes;
}

const std::vector<PlayerObjectTree>& State_ObjectGraph::GetPlayerTrees() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_PlayerTrees;
}

void State_ObjectGraph::SetNodes(std::unordered_map<uint32_t, ObjectNode> nodes)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Nodes = nodes;
}

void State_ObjectGraph::SetPlayerTrees(std::vector<PlayerObjectTree> trees)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_PlayerTrees = trees;
}

void State_ObjectGraph::Cleanup()
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Nodes.clear();
	m_PlayerTrees.clear();
}