#include "pch.h"

#include "State_ObjectGraph.h"

#include "Core/States/Tables/Object/State_ObjectTable.h"

#include <queue>

const std::unordered_map<uint32_t, ObjectNode>& State_ObjectGraph::GetNodes() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_Nodes;
}

void State_ObjectGraph::SetNodes(std::unordered_map<uint32_t, ObjectNode> nodes)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Nodes = nodes;
}

std::vector<uint32_t> State_ObjectGraph::GetChildren(uint32_t handle) const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	std::vector<uint32_t> children;

	auto object = m_State_ObjectTable.CopyLiveObject(handle);
	if (!object || object->ChildHandle == 0xFFFFFFFF) return children;

	uint32_t currentHandle = object->ChildHandle;
	while (currentHandle != 0xFFFFFFFF)
	{
		children.push_back(currentHandle);

		std::optional<LiveObject> child{};
		child = m_State_ObjectTable.CopyLiveObject(currentHandle);
		currentHandle = (child) ? child->NextSiblingHandle : 0xFFFFFFFF;
	}

	return children;
}

uint32_t State_ObjectGraph::GetParent(uint32_t handle) const
{
	std::lock_guard<std::mutex> lock(m_Mutex);

	auto object = m_State_ObjectTable.CopyLiveObject(handle);
	if (!object || object->ParentHandle == 0xFFFFFFFF) return 0xFFFFFFFF;

	return object->ParentHandle;
}

std::vector<uint32_t> State_ObjectGraph::GetSubtree(uint32_t rootHandle) const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	std::vector<uint32_t> result;

	std::queue<uint32_t> pending;
	pending.push(rootHandle);

	while (!pending.empty())
	{
		uint32_t current = pending.front();
		pending.pop();

		result.push_back(current);

		auto object = m_State_ObjectTable.CopyLiveObject(current);
		if (!object) continue;

		uint32_t childHandle = object->ChildHandle;
		while (childHandle != 0xFFFFFFFF)
		{
			pending.push(childHandle);

			std::optional<LiveObject> child{};
			child = m_State_ObjectTable.CopyLiveObject(childHandle);
			if (!child) continue;

			childHandle = child->NextSiblingHandle;
		}
	}

	return result;
}

void State_ObjectGraph::Cleanup()
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Nodes.clear();
}