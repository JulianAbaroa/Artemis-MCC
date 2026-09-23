module Relations.ObjectGraph.State;

namespace Relations::ObjectGraph::State
{
	auto ObjectGraphStore::Publish(ObjectNodes nodes) -> void
	{
		auto snap = std::make_shared<const ObjectNodes>(std::move(nodes));
		m_pObjectNodes.store(snap, std::memory_order_release);
	}

	auto ObjectGraphStore::Acquire() const -> std::shared_ptr<const ObjectNodes>
	{
		return m_pObjectNodes.load(std::memory_order_acquire);
	}

	auto ObjectGraphStore::GetChildren(std::uint32_t handle) const -> std::vector<std::uint32_t>
	{
		auto snap = m_pObjectNodes.load(std::memory_order_acquire);
		if (!snap) return {};
		auto it = snap->find(handle);
		if (it == snap->end()) return {};
		return it->second.ChildrenHandles;
	}

	auto ObjectGraphStore::GetParent(std::uint32_t handle) const -> std::uint32_t
	{
		auto snap = m_pObjectNodes.load(std::memory_order_acquire);
		if (!snap) return 0xFFFFFFFF;
		auto it = snap->find(handle);
		return it != snap->end() ? it->second.ParentHandle : 0xFFFFFFFF;
	}

	auto ObjectGraphStore::GetSubtree(std::uint32_t rootHandle) const -> std::vector<std::uint32_t>
	{
		auto snap = m_pObjectNodes.load(std::memory_order_acquire);
		if (!snap) return {};

		std::vector<std::uint32_t> result;
		std::queue<std::uint32_t> pending;
		pending.push(rootHandle);

		while (!pending.empty())
		{
			std::uint32_t current = pending.front();
			pending.pop();
			result.push_back(current);

			auto it = snap->find(current);
			if (it == snap->end()) continue;

			for (std::uint32_t child : it->second.ChildrenHandles)
			{
				pending.push(child);
			}
		}

		return result;
	}

	auto ObjectGraphStore::Cleanup() -> void
	{
		m_pObjectNodes.store(nullptr, std::memory_order_release);
	}
}