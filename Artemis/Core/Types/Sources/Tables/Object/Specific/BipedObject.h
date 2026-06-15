#pragma once

#include "Core/Types/Team.h"
#include <array>

enum class ZoomLevel : uint8_t
{
	None = 0xFF,
	Zoom1 = 0x00,
	Zoom2 = 0x01,
};

struct BipedObject
{
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