#include "pch.h"

#include "System_ObjectGraph.h"

#include "Core/States/Tables/Object/State_ObjectTable.h"
#include "Core/States/Filtered/Graph/Object/State_ObjectGraph.h"

#include "Core/Systems/Logs/System_Logs.h"

// Called from 'Thread_AI::Run', its responsible for generating & updating
// the father-child-sibling relationship between objects. These relationships
// are called ObjectNodes and PlayerTrees.
void System_ObjectGraph::UpdateGraph()
{
	std::unordered_map<uint32_t, ObjectNode> nodes;
	this->BuildNodes(nodes);
	m_Deps.State_ObjectGraph.SetNodes(std::move(nodes));
}

void System_ObjectGraph::Cleanup()
{
	m_Deps.State_ObjectGraph.Cleanup();
	m_Deps.System_Logs.Log("[System_ObjectGraph] INFO: Cleanup completed.");
}

void System_ObjectGraph::BuildNodes(std::unordered_map<uint32_t, ObjectNode>& nodes)
{
	// Get the entire Artemis object table.
	const auto& objectTable = m_Deps.State_ObjectTable.GetObjectTable();

	for (const auto& [handle, object] : objectTable)
	{
		ObjectNode node{};
		node.Handle = handle;
		node.ParentHandle = object.ParentHandle;
		
		uint32_t childHandle = object.ChildHandle;
		while (childHandle != 0xFFFFFFFF)
		{
			auto it = objectTable.find(childHandle);
			if (it == objectTable.end()) break;

			node.ChildrenHandles.push_back(childHandle);
			childHandle = it->second.NextSiblingHandle;
		}

		nodes.emplace(handle, std::move(node));
	}
}