#pragma once

// Types.
#include "Core/Types/Domain/Object/LiveObject.h"

// Memory Reader.
#include "Core/Systems/Infrastructure/Engine/Memory/System_MemoryReader.h"

#include <unordered_map>
#include <cstdint>
#include <atomic>
#include <mutex>

class System_ObjectTable
{
public:
	void OnObjectCreated(uint32_t handle, uint32_t datumIndex);
	void OnObjectDestroyed(uint32_t handle);

	void FindObjectTableBase();
	void UpdateObjectTable();

	void Cleanup();

private:
	void UpdateObjectData(LiveObject& object);

	void UpdateBiped(System_MemoryReader& reader, LiveObject& object);
	void UpdateVehicle(System_MemoryReader& reader, LiveObject& object);
	void UpdateWeapon(System_MemoryReader& reader, LiveObject& object);
	void UpdateEquipment(System_MemoryReader& reader, LiveObject& object);
	void UpdateProjectiles(System_MemoryReader& reader, LiveObject& object);
	void UpdateCrate(System_MemoryReader& reader, LiveObject& object);
	void UpdateScenery(System_MemoryReader& reader, LiveObject& object);

	// --- Helpers ---

	ObjectClass ClassNameToObjectClass(const std::string& className);
};