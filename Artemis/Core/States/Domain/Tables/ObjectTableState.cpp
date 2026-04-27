#include "pch.h"
#include "Core/States/Domain/Tables/ObjectTableState.h"

const LiveObject* ObjectTableState::GetObject(uint32_t handle) const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	auto it = m_Objects.find(handle);
	return it != m_Objects.end() ? &it->second : nullptr;
}

const std::unordered_map<uint32_t, LiveObject> ObjectTableState::GetObjectTable() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_Objects;
}

void ObjectTableState::AddObject(uint32_t handle, const LiveObject& object)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Objects[handle] = object;
	m_HasChanged.store(true);
}

std::optional<LiveObject> ObjectTableState::RemoveObject(uint32_t handle)
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

void ObjectTableState::UpdateObjects(std::function<void(uint32_t, LiveObject&)> processor)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	for (auto& [handle, object] : m_Objects)
	{
		processor(handle, object);
	}
	m_HasChanged.store(true);
}

bool ObjectTableState::HasChanged() const { return m_HasChanged.load(); }
void ObjectTableState::SetChanged(bool value) { m_HasChanged.store(value); }

uintptr_t ObjectTableState::GetObjectTableBase() { return m_ObjectTableBase.load(); }
void ObjectTableState::SetObjectTableBase(uintptr_t pointer) { m_ObjectTableBase.store(pointer); }

void ObjectTableState::Cleanup()
{
	m_HasChanged.store(false);
	m_ObjectTableBase.store(0);

	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Objects.clear();
}