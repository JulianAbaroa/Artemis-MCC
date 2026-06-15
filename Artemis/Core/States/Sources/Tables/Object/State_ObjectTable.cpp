#include "pch.h"

#include "State_ObjectTable.h"

uintptr_t State_ObjectTable::GetBase()
{
	return m_ObjectTableBase.load();
}

void State_ObjectTable::SetBase(uintptr_t pointer)
{
	m_ObjectTableBase.store(pointer);
}

void State_ObjectTable::AddObject(uint32_t handle, 
	const LiveObject& object)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_ObjectTable.emplace(handle, object);
}

std::optional<LiveObject> State_ObjectTable::RemoveObject(
	uint32_t handle)
{
	std::lock_guard<std::mutex> lock(m_Mutex);

	auto it = m_ObjectTable.find(handle);
	if (it != m_ObjectTable.end())
	{
		LiveObject removedCopy = std::move(it->second);
		m_ObjectTable.erase(handle);
		return removedCopy;
	}

	return std::nullopt;
}

void State_ObjectTable::UpdateObjects(std::function<void(uint32_t, LiveObject&)> processor)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	for (auto& [handle, object] : m_ObjectTable)
	{
		processor(handle, object);
	}
}

void State_ObjectTable::Publish()
{
	std::shared_ptr<const ObjectTable> snap;
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		snap = std::make_shared<const ObjectTable>(m_ObjectTable);
	}
	m_pObjectTable.store(snap, std::memory_order_release);
}

std::shared_ptr<const ObjectTable> State_ObjectTable::Acquire() const
{
	return m_pObjectTable.load(std::memory_order_acquire);
}

void State_ObjectTable::Cleanup()
{
	m_ObjectTableBase.store(0);
	m_pObjectTable.store(nullptr, std::memory_order_release);

	std::lock_guard<std::mutex> lock(m_Mutex);
	m_ObjectTable.clear();
}