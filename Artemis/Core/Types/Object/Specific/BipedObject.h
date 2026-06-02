#pragma once

#include "../../Team.h"
#include <cstdint>
#include <array>

enum class ZoomLevel : uint8_t
{
	None = 0xFF,
	Zoom1 = 0x00,
	Zoom2 = 0x01,
};

// Representation of a biped object, alive in game engine's memory.
struct BipedObject
{
	// Health.
	float Shields;
	float Health;
	uint8_t HasNoShields;
	uint16_t HealthRegenerationDelay;
	uint16_t ShieldsRegenerationDelay;

	// Movement.
	std::array<float, 2> MovementDirection;
	std::array<float, 3> SurfaceNormal;
	uint32_t GroundObjectHandle;
	uint16_t MaterialIndex;

	// States.
	uint8_t IsCrouched;
	uint8_t IsGrounded;
	uint8_t IsAbilityActive;	
	ZoomLevel ZoomLevel;

	// Damager.
	uint32_t DamagerBipedHandle;
	uint32_t DamagerPlayerHandle;
};