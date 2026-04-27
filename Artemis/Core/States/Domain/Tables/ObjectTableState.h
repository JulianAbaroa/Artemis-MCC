#pragma once

#include "Core/Common/Types/Domain/Tables/ObjectTypes.h"
#include <unordered_map>
#include <functional>
#include <optional>
#include <cstdint>
#include <atomic>
#include <mutex>

class ObjectTableState
{
public:
	const LiveObject* GetObject(uint32_t handle) const;
	const std::unordered_map<uint32_t, LiveObject> GetObjectTable() const;
	void AddObject(uint32_t handle, const LiveObject& object);
	std::optional<LiveObject> RemoveObject(uint32_t handle);
	void UpdateObjects(std::function<void(uint32_t, LiveObject&)> processor);

	bool HasChanged() const;
	void SetChanged(bool value);

	uintptr_t GetObjectTableBase();
	void SetObjectTableBase(uintptr_t pointer);

	void Cleanup();

private:
	std::unordered_map<uint32_t, LiveObject> m_Objects;
	mutable std::mutex m_Mutex;

	std::atomic<bool> m_HasChanged = false;
	std::atomic<intptr_t> m_ObjectTableBase{ 0 };
};