#pragma once

#include <cstdint>
#include <vector>

struct PlayerTree
{
	uint32_t Handle;

	uint32_t BipedHandle = 0xFFFFFFFF;
	uint32_t PrimaryWeaponHandle = 0xFFFFFFFF;
	uint32_t SecondaryWeaponHandle = 0xFFFFFFFF;
	uint32_t AbilityHandle = 0xFFFFFFFF;
	uint32_t ObjectiveHandle = 0xFFFFFFFF;
	uint32_t ParentHandle = 0xFFFFFFFF;

	std::vector<uint32_t> VehiclePartHandles;

	bool IsAlive() const 
	{ 
		return BipedHandle != 0xFFFFFFFF; 
	}

	bool IsInVehicle() const 
	{ 
		return ParentHandle != 0xFFFFFFFF; 
	}
};