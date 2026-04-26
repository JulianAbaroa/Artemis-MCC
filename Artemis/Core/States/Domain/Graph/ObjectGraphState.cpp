#include "pch.h"
#include "Core/States/Domain/Graph/ObjectGraphState.h"

const std::unordered_map<uint32_t, ObjectNode>& ObjectGraphState::GetNodes() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_Nodes;
}

const std::vector<PlayerObjectTree>& ObjectGraphState::GetPlayerTrees() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_PlayerTrees;
}

void ObjectGraphState::SetNodes(std::unordered_map<uint32_t, ObjectNode> nodes)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Nodes = nodes;
}

void ObjectGraphState::SetPlayerTrees(std::vector<PlayerObjectTree> trees)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_PlayerTrees = trees;
}

void ObjectGraphState::Cleanup()
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Nodes.clear();
	m_PlayerTrees.clear();
}