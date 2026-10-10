module Relations.ObjectGraph.System;

import Tables.Object.Type;

namespace
{
	using AliveObject = Tables::Object::Type::Alive::Object;
	using ObjectTable = std::unordered_map<std::uint32_t, AliveObject>;
}

namespace Relations::ObjectGraph::System
{
	auto ObjectGraphService::UpdateGraph() -> void
	{
		ObjectNodes nodes;
		this->BuildNodes(nodes);
		m_ObjectGraphStore.Publish(std::move(nodes));
	}

	auto ObjectGraphService::BuildNodes(ObjectNodes& nodes) -> void
	{
		auto objectTablePtr = m_ObjectStore.Acquire();
		if (!objectTablePtr) return;
		const ObjectTable& objectTable = *objectTablePtr;

		for (const auto& [handle, object] : objectTable)
		{
			ObjectNode node{};
			node.Handle = handle;
			node.ParentHandle = object.ParentHandle;

			std::uint32_t childHandle = object.ChildHandle;
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

	auto ObjectGraphService::Cleanup() -> void
	{
		m_ObjectGraphStore.Cleanup();

		m_LogsService.Message("[ObjectGraphService] INFO: Cleanup completed.");
	}
}