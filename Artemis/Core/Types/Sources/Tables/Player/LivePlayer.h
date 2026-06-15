#pragma once

#include "Core/Types/Team.h"
#include <string>
#include <array>

enum class ConnectionState : uint8_t
{
	Connected = 0x01,
	Disconnected = 0x02,
	Connecting = 0x08,
};

struct LivePlayer
{
	uint32_t Handle;
	uintptr_t Address;

	ConnectionState ConnectionState;
	Team Team;

	std::string Gamertag;
	std::string Tag;

	std::array<float, 3> WeaponPosition;
	std::array<float, 3> WeaponForward;
	std::array<float, 3> AimOffset;

	uint32_t PrimaryWeaponHandle;
	uint32_t SecondaryWeaponHandle;
	uint32_t ObjectiveHandle;

	uint32_t AliveBipedHandle;
	uint32_t DeadBipedHandle;
	uint32_t CurrentBipedHandle;
};