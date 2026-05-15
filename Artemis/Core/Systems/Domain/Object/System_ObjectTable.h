#pragma once

// Types.
#include "Core/Types/Domain/Object/LiveObject.h"

#include <unordered_map>
#include <cstdint>
#include <atomic>
#include <mutex>

// Dependencies.
class State_ObjectTable;
class System_Map;
class System_MemoryReader;
class System_Debug;

//class State_Map;

class State_MapPhmo;
class State_MapColl;
class State_MapMode;
class State_MapJmad;
class State_MapVehi;
class State_MapBloc;
class State_MapSbsp;
class State_MapWeap;
class State_MapProj;
class State_MapBipd;
class State_MapEqip;
class State_MapScen;
class State_MapScnr;
class State_MapCtrl;
class State_MapMach;

struct System_ObjectTable_Dependencies
{
	System_Map& System_Map;
	System_MemoryReader& System_MemoryReader;
	State_ObjectTable& State_ObjectTable;

	// Hlmt-derived states
	State_MapPhmo& State_MapPhmo;
	State_MapColl& State_MapColl;
	State_MapMode& State_MapMode;
	State_MapJmad& State_MapJmad;

	// Direct tag states
	State_MapVehi& State_MapVehi;
	State_MapBloc& State_MapBloc;
	State_MapSbsp& State_MapSbsp;
	State_MapWeap& State_MapWeap;
	State_MapProj& State_MapProj;
	State_MapBipd& State_MapBipd;
	State_MapEqip& State_MapEqip;
	State_MapScen& State_MapScen;
	State_MapScnr& State_MapScnr;
	State_MapCtrl& State_MapCtrl;
	State_MapMach& State_MapMach;

	System_Debug& System_Debug;
};

class System_ObjectTable
{
public:
	System_ObjectTable(System_ObjectTable_Dependencies dependencies) :
		m_Dependencies(dependencies) {}
	~System_ObjectTable() = default;

	void OnObjectCreated(uint32_t handle, uint32_t datumIndex);
	void OnObjectDestroyed(uint32_t handle);

	void UpdateObjectTable();

	void Cleanup();

private:
	System_ObjectTable_Dependencies m_Dependencies;

	void UpdateObjectData(LiveObject& object);

	void UpdateBiped(System_MemoryReader& reader, LiveObject& object);
	void UpdateVehicle(System_MemoryReader& reader, LiveObject& object);
	void UpdateWeapon(System_MemoryReader& reader, LiveObject& object);
	void UpdateEquipment(System_MemoryReader& reader, LiveObject& object);
	void UpdateProjectiles(System_MemoryReader& reader, LiveObject& object);
	void UpdateCrate(System_MemoryReader& reader, LiveObject& object);
	void UpdateScenery(System_MemoryReader& reader, LiveObject& object);
};