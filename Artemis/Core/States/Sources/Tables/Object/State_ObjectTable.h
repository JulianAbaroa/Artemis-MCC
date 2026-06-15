#pragma once

#include "Core/Types/Sources/Tables/Object/LiveObject.h"

#include <functional>
#include <atomic>
#include <mutex>

using ObjectTable = std::unordered_map<uint32_t, LiveObject>;

class State_ObjectTable
{
public:
	uintptr_t GetBase();
	void SetBase(uintptr_t pointer);

	void AddObject(uint32_t handle, const LiveObject& object);
	std::optional<LiveObject> RemoveObject(uint32_t handle);

	void UpdateObjects(std::function<void(uint32_t, LiveObject&)> processor);

	void Publish();
	std::shared_ptr<const ObjectTable> Acquire() const;

	void Cleanup();

private:
	std::atomic<intptr_t> m_ObjectTableBase{ 0 };
	std::atomic<std::shared_ptr<const ObjectTable>> m_pObjectTable;

	std::unordered_map<uint32_t, LiveObject> m_ObjectTable;
	mutable std::mutex m_Mutex;
};