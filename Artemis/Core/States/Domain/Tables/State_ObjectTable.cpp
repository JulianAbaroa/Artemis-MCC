#include "pch.h"
#include "Core/States/Domain/Tables/State_ObjectTable.h"

const LiveObject* State_ObjectTable::GetObject(uint32_t handle) const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	auto it = m_Objects.find(handle);
	return it != m_Objects.end() ? &it->second : nullptr;
}

const std::unordered_map<uint32_t, LiveObject> State_ObjectTable::GetObjectTable() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_Objects;
}

void State_ObjectTable::AddObject(uint32_t handle, const LiveObject& object)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Objects[handle] = object;
	m_HasChanged.store(true);
}

std::optional<LiveObject> State_ObjectTable::RemoveObject(uint32_t handle)
{
	std::lock_guard<std::mutex> lock(m_Mutex);

	auto it = m_Objects.find(handle);
	if (it != m_Objects.end())
	{
		LiveObject removedCopy = std::move(it->second);
		m_Objects.erase(handle);
		m_HasChanged.store(true);
		return removedCopy;
	}

	return std::nullopt;
}

void State_ObjectTable::UpdateObjects(std::function<void(uint32_t, LiveObject&)> processor)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	for (auto& [handle, object] : m_Objects)
	{
		processor(handle, object);
	}
	m_HasChanged.store(true);
}

bool State_ObjectTable::HasChanged() const { return m_HasChanged.load(); }
void State_ObjectTable::SetChanged(bool value) { m_HasChanged.store(value); }

uintptr_t State_ObjectTable::GetObjectTableBase() { return m_ObjectTableBase.load(); }
void State_ObjectTable::SetObjectTableBase(uintptr_t pointer) { m_ObjectTableBase.store(pointer); }

void State_ObjectTable::Cleanup()
{
	m_HasChanged.store(false);
	m_ObjectTableBase.store(0);

	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Objects.clear();
}