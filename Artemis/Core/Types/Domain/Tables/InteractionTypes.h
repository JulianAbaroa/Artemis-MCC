#pragma once

#include <cstdint>
#include <cstddef>

#pragma pack(push, 1)

// Raw memory layout of the engine's interaction table entry.
// Base address resolved once at engine init via InteractionTableHook.
struct InteractionTable
{
	uint8_t InteractionType;			// 0x00
	uint8_t UnknownFlag01;				// 0x01			
	uint16_t UnknownFlags04;			// 0x02
	uint8_t InteractionSlotID;			// 0x04 | 00 = Driver, 01 = Passenger. 01 = Grab Weapon, 02 = Change weapon.
	uint8_t UnknownFlag05;				// 0x05
	uint16_t UnknownFlags06;			// 0x06
	uint32_t TargetObjectHandle;		// 0x08
	uint8_t IsMeleeAvailable;			// 0x0C | Is '0E' when a meele is available.
	uint8_t UnknownFlag0A;				// 0x0D
	uint16_t UnknownFlags0B;			// 0x0E
	uint32_t UnknownFlags10;			// 0x10
	uint32_t MeleeTargetHandle;			// 0x14 | When a melee is available, this exists.
	uint32_t UnknownFlags18;			// 0x18
	uint32_t UnknownFlags1C;			// 0x1C
	uint32_t UnknownHandle20;			// 0x20
	uint8_t IsAimAvailable;				// 0x24 | Is '01' when the crosshair is somewhere close to a biped.
	uint8_t UnknownFlag25;				// 0x25 
	uint16_t UnknownFlags26;			// 0x26
	uint8_t BodyPart;					// 0x28 | Chest: 00, Head: 01, Left leg: 02, Right leg: 03. Spartans is 01 head, 00 chest, marines is 00 head, 01 chest. How it is with elites: 01 head. grunts: 01 head. jackals? etcetera.
	uint8_t UnknownFlag29;				// 0x29 
	uint16_t UnknownFlags2A;			// 0x2A
	uint32_t AimTargetHandle;			// 0x2C	| When aiming a player is available, this exists.
	uint32_t AimTargetSlotID;			// 0x30 | Target player SlotID on PlayerTable.
	uint32_t UnknownFlags34;			// 0x34
	uint32_t UnknownFlags38;			// 0x38
	float AimHitLocalPosition[3];		// 0x3C | Smaller values indicates we're aiming closer to the center of this biped.
	uint8_t UnknownFlag48;				// 0x48
	uint8_t UnknownFlag49;				// 0x49
	uint16_t UnknownFlags4A;			// 0x4A
	uint32_t UnknownFlags4C;			// 0x4C
	std::byte _pad_50[92];				// 0x50
};

#pragma pack(pop)

// Known interaction types read from InteractionTable::InteractionType.
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

enum class BodyPart : uint8_t
{
	Chest = 0x00,
	Head = 0x01,
	LeftLeg = 0x02,
	RightLeg = 0x03,
};

// High-level snapshot of the engine's current interaction state.
struct LiveInteraction
{
	// Objects.
	InteractionType Type = InteractionType::None;
	uint8_t InteractionSlotID = 0x00;
	uint32_t TargetObjectHandle = 0xFFFFFFFF;

	// Players: Melee.
	uint8_t IsMeleeAvailable = 0x00;
	uint32_t MeleeTargetHandle = 0xFFFFFFFF;

	// Players: Aim.
	uint8_t IsAimAvailable = 0x00;
	BodyPart BodyPart = BodyPart::Head;
	uint32_t AimTargetHandle = 0xFFFFFFFF;
	uint32_t AimTargetSlotID = 0xFFFFFFFF;
	float AimHitLocalPosition[3]{};
};