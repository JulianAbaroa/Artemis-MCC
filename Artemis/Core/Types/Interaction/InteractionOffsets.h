#pragma once

#include <cstdint>

namespace InteractionOffsets
{
	// What kind of interaction is active. (GrabWeapon, EnterVehicle, etcetera)
	constexpr uintptr_t InteractionType = 0x00;		// uint8.

	// Interaction detail, dependant on the interaction type.
	// 0x00 = Driver, 0x01 = Passenger or 0x01 = Grab Weapon, 0x02 = Change Weapon.
	constexpr uintptr_t InteractionDetail = 0x04;	// uint8.

	// The handle of the selected object to interact with.
	constexpr uintptr_t TargetObjectHandle = 0x08;	// uint32.

	// A flag that is 0x01 when a melee hit is available.
	// This doesn't include hammer/sword melees.
	// Its 0x0E when there's a melee available.
	constexpr uintptr_t IsMeleeAvailable = 0x0C;	// uint8.

	// The handle of the selected biped to melee.
	constexpr uintptr_t MeleeTargetHandle = 0x14;	// uint32.

	// A flag that is 0x01 when a biped is close or on to the crosshair.
	constexpr uintptr_t IsAimAvailable = 0x24;		// uint8.

	// Each flag represents a part of thee body of any given biped.
	// 0x00: Chest, 0x01: Head, etcetera.
	constexpr uintptr_t BipedBodyPart = 0x28;		// unit8.

	// The handle of the selected biped.
	constexpr uintptr_t AimTargetHandle = 0x2C;		// uint32.

	// The slot ID of the targeted player (PlayerOffsets::Handle).
	constexpr uintptr_t AimTargetSlotID = 0x30;		// uint32.

	// The offset of how far the crosshair is from the center of the target.
	// Higher values = farther from the center.
	// Smaller valuees = closer to the center.
	constexpr uintptr_t AimHitLocalPosition = 0x3C;	// 3 floats.
}