#pragma once

#include "../../Team.h"
#include <optional>
#include <cstdint>

enum class ActionState : uint8_t
{
	Idle = 0x00,
	Firing = 0x02,
	Meleeing = 0x60,
	Zoomed = 0x80,
};

// Representation of a weapon object, alive in game engine's memory.
struct WeaponObject
{
	// Ammo-related
	float TotalHeat;
	float TotalEnergy;
	uint16_t TotalAmmo;
	uint16_t CurrentAmmo;

	// State-related.
	uint8_t IsFiring;
	uint8_t IsReloading;
	uint8_t ChargeProgress;
	ActionState ActionState;

	std::optional<Team> Team;
};