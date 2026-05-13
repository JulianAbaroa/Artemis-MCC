#include "pch.h"

// Header.
#include "State_ObjectGraph.h"

// States.
#include "Core/States/Core_State.h"
#include "Core/States/Domain/Core_State_Domain.h"

#include "Core/States/Domain/Object/State_ObjectTable.h"

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

	auto& objectTable = *g_pState->Domain->ObjectTable;

	auto object = objectTable.CopyLiveObject(handle);
	if (!object || object->ChildHandle == 0xFFFFFFFF) return children;

	uint32_t currentHandle = object->ChildHandle;
	while (currentHandle != 0xFFFFFFFF)
	{
		children.push_back(currentHandle);

		auto child = objectTable.CopyLiveObject(currentHandle);
		if (!child) break;

		currentHandle = child->NextSiblingHandle;
	}

	return children;
}

uint32_t State_ObjectGraph::GetParent(uint32_t handle) const
{
	std::lock_guard<std::mutex> lock(m_Mutex);

	auto& objectTable = *g_pState->Domain->ObjectTable;

	auto object = objectTable.CopyLiveObject(handle);
	if (!object || object->ParentHandle == 0xFFFFFFFF) return 0xFFFFFFFF;

	return object->ParentHandle;
}

std::vector<uint32_t> State_ObjectGraph::GetSubtree(uint32_t rootHandle) const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	std::vector<uint32_t> result;

	std::queue<uint32_t> pending;
	pending.push(rootHandle);

	auto& objectTable = *g_pState->Domain->ObjectTable;

	while (!pending.empty())
	{
		uint32_t current = pending.front();
		pending.pop();

		result.push_back(current);

		auto object = objectTable.CopyLiveObject(current);
		if (!object) continue;

		uint32_t childHandle = object->ChildHandle;
		while (childHandle != 0xFFFFFFFF)
		{
			pending.push(childHandle);

			auto child = objectTable.CopyLiveObject(childHandle);
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