#pragma once

#include "../Team.h"
#include <cstdint>
#include <string>
#include <array>

// The connection state of any given player.
// There are unknown values like 0x09 & 0x0A that needs to be studied.
enum class ConnectionState : uint8_t
{
	Connected = 0x01,
	Disconnected = 0x02,
	Connecting = 0x08,
};

// Representation of a player inside the player table.
struct LivePlayer
{
	uint32_t Handle;
	uintptr_t Address;

	ConnectionState ConnectionState;
	bool IsAlive = false;
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