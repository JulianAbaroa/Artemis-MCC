#pragma once

// Types.
#include "Core/Types/Domain/Graph/ObjectNode.h"

#include <unordered_map>
#include <cstdint>
#include <vector>
#include <mutex>

class State_ObjectTable;

class State_ObjectGraph
{
public:
	State_ObjectGraph(State_ObjectTable& stateObjectTable) : 
		m_State_ObjectTable(stateObjectTable) {}
	~State_ObjectGraph() = default;

	const std::unordered_map<uint32_t, ObjectNode>& GetNodes() const;
	void SetNodes(std::unordered_map<uint32_t, ObjectNode> nodes);

	std::vector<uint32_t> GetChildren(uint32_t handle) const;
	uint32_t GetParent(uint32_t handle) const;
	std::vector<uint32_t> GetSubtree(uint32_t rootHandle) const;

	void Cleanup();

private:
	State_ObjectTable& m_State_ObjectTable;

	std::unordered_map<uint32_t, ObjectNode> m_Nodes;
	mutable std::mutex m_Mutex;
};