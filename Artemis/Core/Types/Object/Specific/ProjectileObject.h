#pragma once

#include <cstdint>

enum class ProjectileFlags : uint32_t
{
	IsTracking = 0x00000100,
	IsCrit = 0x00400000,
	IsAltFire = 0x00800000,
};

// Representation of a projectile object, alive in game engine's memory.
struct ProjectileObject
{
	ProjectileFlags ProjectileFlags;
	uint32_t OwnerBipedHandle;
	uint32_t OwnerWeaponHandle;
};