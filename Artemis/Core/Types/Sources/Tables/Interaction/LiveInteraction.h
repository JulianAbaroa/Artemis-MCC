#pragma once

#include <cstdint>
#include <array>

enum class InteractionType : uint8_t
{
	None = 0x00,
	GrabWeapon = 0x02,
	GrabArmorAbility = 0x03,
	TakeHealthStation = 0x04,
	EnterVehicle = 0x06,
	Hijack = 0x09,
	GrabObjective = 0x0A,
};

enum class InteractionDetail : uint8_t
{
	None = 0xFF,
	ZeroSeat = 0x00,
	FirstSeat = 0x01,
	SecondSeat = 0x02,
	ThirdSeat = 0x03,
	FourthSeat = 0x04,
	FifthSeat = 0x05,
	GrabWeapon = 0x01,
	ChangeWeapon = 0x02
};

struct LiveInteraction
{
	// Objects.
	InteractionType Type = InteractionType::None;
	InteractionDetail InteractionSlotID = InteractionDetail::None;
	uint32_t TargetObjectHandle = 0xFFFFFFFF;

	// Players: Melee.
	uint8_t IsMeleeAvailable = 0x00;
	uint32_t MeleeTargetHandle = 0xFFFFFFFF;

	// Players: Aim.
	uint8_t IsAimAvailable = 0x00;
	uint8_t ModelPart = 0x00; // TODO: Connect with mode tag data.
	uint32_t AimTargetHandle = 0xFFFFFFFF;
	uint32_t AimTargetSlotID = 0xFFFFFFFF;
	std::array<float, 3> AimHitLocalPosition{};
};