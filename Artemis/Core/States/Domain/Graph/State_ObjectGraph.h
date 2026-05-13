#pragma once

// Types.
#include "Core/Types/Domain/Graph/ObjectNode.h"

#include <unordered_map>
#include <cstdint>
#include <vector>
#include <mutex>

class State_ObjectGraph
{
public:
	const std::unordered_map<uint32_t, ObjectNode>& GetNodes() const;
	void SetNodes(std::unordered_map<uint32_t, ObjectNode> nodes);

	std::vector<uint32_t> GetChildren(uint32_t handle) const;
	uint32_t GetParent(uint32_t handle) const;
	std::vector<uint32_t> GetSubtree(uint32_t rootHandle) const;

	void Cleanup();

private:
	std::unordered_map<uint32_t, ObjectNode> m_Nodes;
	mutable std::mutex m_Mutex;
};