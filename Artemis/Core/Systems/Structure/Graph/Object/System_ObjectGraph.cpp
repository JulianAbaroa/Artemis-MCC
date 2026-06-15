#include "pch.h"

#include "System_ObjectGraph.h"

#include "Core/States/Sources/Tables/Object/State_ObjectTable.h"
#include "Core/States/Structure/Graph/Object/State_ObjectGraph.h"

#include "Core/Systems/Other/Logs/System_Logs.h"

void System_ObjectGraph::UpdateGraph()
{
	ObjectGraph nodes;
	this->BuildNodes(nodes);
	m_Deps.State_ObjectGraph.Publish(std::move(nodes));
}

void System_ObjectGraph::BuildNodes(ObjectGraph& nodes)
{
	auto objectTablePtr = m_Deps.State_ObjectTable.Acquire();
	if (!objectTablePtr) return;
	const ObjectTable& objectTable = *objectTablePtr;

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

void System_ObjectGraph::Cleanup()
{
	m_Deps.State_ObjectGraph.Cleanup();

	m_Deps.System_Logs.Log("[ObjectGraph] INFO:"
		" Cleanup completed.");
}