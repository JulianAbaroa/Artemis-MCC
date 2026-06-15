#include "pch.h"

#include "State_ObjectGraph.h"

#include <queue>

void State_ObjectGraph::Publish(ObjectGraph nodes)
{
	auto snap = std::make_shared<const ObjectGraph>(std::move(nodes));
	m_pObjectGraph.store(snap, std::memory_order_release);
}

std::shared_ptr<const ObjectGraph> State_ObjectGraph::Acquire() const
{
	return m_pObjectGraph.load(std::memory_order_acquire);
}

std::vector<uint32_t> State_ObjectGraph::GetChildren(uint32_t handle) const
{
	auto snap = m_pObjectGraph.load(std::memory_order_acquire);
	if (!snap) return {};
	auto it = snap->find(handle);
	if (it == snap->end()) return {};
	return it->second.ChildrenHandles;
}

uint32_t State_ObjectGraph::GetParent(uint32_t handle) const
{
	auto snap = m_pObjectGraph.load(std::memory_order_acquire);
	if (!snap) return 0xFFFFFFFF;
	auto it = snap->find(handle);
	return it != snap->end() ? it->second.ParentHandle : 0xFFFFFFFF;
}

std::vector<uint32_t> State_ObjectGraph::GetSubtree(uint32_t rootHandle) const
{
	auto snap = m_pObjectGraph.load(std::memory_order_acquire);
	if (!snap) return {};

	std::vector<uint32_t> result;
	std::queue<uint32_t> pending;
	pending.push(rootHandle);

	while (!pending.empty())
	{
		uint32_t current = pending.front();
		pending.pop();
		result.push_back(current);

		auto it = snap->find(current);
		if (it == snap->end()) continue;

		for (uint32_t child : it->second.ChildrenHandles)
		{
			pending.push(child);
		}
	}

	return result;
}

void State_ObjectGraph::Cleanup()
{
	m_pObjectGraph.store(nullptr, std::memory_order_release);
}