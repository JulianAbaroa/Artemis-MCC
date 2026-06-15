#pragma once

#include "Core/Types/Structure/Graph/Object/ObjectNode.h"

#include <unordered_map>
#include <memory>
#include <atomic>

using ObjectGraph = std::unordered_map<uint32_t, ObjectNode>;

class State_ObjectGraph
{
public:
	void Publish(ObjectGraph nodes);
	std::shared_ptr<const ObjectGraph> Acquire() const;

	std::vector<uint32_t> GetChildren(uint32_t handle) const;
	uint32_t GetParent(uint32_t handle) const;
	std::vector<uint32_t> GetSubtree(uint32_t rootHandle) const;

	void Cleanup();

private:
	std::atomic<std::shared_ptr<const ObjectGraph>> m_pObjectGraph;
};