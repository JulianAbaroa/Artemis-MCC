module Tables.Object.State;

namespace Tables::Object::State
{
	auto ObjectTableStore::GetBase() -> std::uintptr_t
	{
		return m_ObjectTableBase.load();
	}

	auto ObjectTableStore::SetBase(std::uintptr_t pointer) -> void
	{
		m_ObjectTableBase.store(pointer);
	}

	auto ObjectTableStore::AddObject(std::uint32_t handle, const AliveObject& object) -> void
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_ObjectTable.emplace(handle, object);
	}

	auto ObjectTableStore::RemoveObject(std::uint32_t handle) -> std::optional<AliveObject>
	{
		std::lock_guard<std::mutex> lock(m_Mutex);

		auto it = m_ObjectTable.find(handle);
		if (it != m_ObjectTable.end())
		{
			AliveObject removedCopy = std::move(it->second);
			m_ObjectTable.erase(handle);
			return removedCopy;
		}

		return std::nullopt;
	}

	auto ObjectTableStore::UpdateObjects(std::function<void(std::uint32_t,
		AliveObject&)> processor) -> void
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		for (auto& [handle, object] : m_ObjectTable)
		{
			processor(handle, object);
		}
	}

	auto ObjectTableStore::Publish() -> void
	{
		std::shared_ptr<const ObjectTable> snap;
		{
			std::lock_guard<std::mutex> lock(m_Mutex);
			snap = std::make_shared<const ObjectTable>(m_ObjectTable);
		}
		m_pObjectTable.store(snap, std::memory_order_release);
	}

	auto ObjectTableStore::Acquire() const -> std::shared_ptr<const ObjectTable>
	{
		return m_pObjectTable.load(std::memory_order_acquire);
	}

	auto ObjectTableStore::Cleanup() -> void
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
	
		m_ObjectTable.clear();
		m_ObjectTableBase.store(0);
		m_pObjectTable.store(nullptr, std::memory_order_release);
	}
}