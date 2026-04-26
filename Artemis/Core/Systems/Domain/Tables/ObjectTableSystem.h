#pragma once

#include "Core/Common/Types/Domain/Tables/ObjectTypes.h"
#include <unordered_map>
#include <cstdint>
#include <atomic>
#include <mutex>

class ObjectTableSystem
{
public:
	void OnObjectCreated(uint32_t handle, uint32_t datumIndex);
	void OnObjectDestroyed(uint32_t handle);

	void FindObjectTableBase();
	void UpdateObjectTable();

	void Cleanup();

private:
	void UpdateObjectData(LiveObject& object);
};