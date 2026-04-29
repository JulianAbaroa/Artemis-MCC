#pragma once

#include <variant>
#include <cstdint>
#include <cstddef>
#include <string>

#pragma pack(push, 1)

// Raw representation of an object inside the ObjectTable
struct ObjectHeader
{
	uint32_t DatumIndex;				// 0x000
	uint32_t UnknownFlags004;			// 0x004
	uint32_t UnknownFlags008;			// 0x008
	uint32_t NextSiblingHandle;			// 0x00C

	uint32_t ChildHandle;				// 0x010		
	uint32_t ParentHandle;				// 0x014
	uint32_t UnknownFlags018;			// 0x018
	uint32_t UnknownFlags01C;			// 0x01C

	float CurrentPosition[3];			// 0x020-0x024-0x028
	float CurrentRadius;				// 0x02C

	float LastNotifiedPosition[3];		// 0x030-0x034-0x038
	float LastNotifiedRadius;			// 0x03C

	uint32_t ContainerHandle;			// 0x040
	float PhysicsPosition[3];			// 0x044-0x048-0x04C

	float Forward[3];					// 0x050-0x054-0x058
	float Up[3];						// 0x05C-0x060-0x064

	float LinearVelocity[3];			// 0x068-0x06C-0x070
	float AngularVelocity[3];			// 0x074-0x078-0x07C

	float KinematicValid;				// 0x080
	float ObjectScale;					// 0x084
	uint32_t ObjectType;				// 0x088
	uint32_t UnknownFlags08C;			// 0x08C

	uint32_t UnknownFlags090;			// 0x090
	uint32_t UnknownFlags094;			// 0x094
	uint32_t UnknownHandle098;			// 0x098
	uint32_t UnknownHandle09C;			// 0x09C

	uint32_t UnknownHandle100;			// 0x100
	uint32_t UnknownHandle104;			// 0x104
	uint32_t UnknownHandle108;			// 0x108
	uint32_t UnknownHandle10C;			// 0x10C

	uint32_t UnknownHandle110;			// 0x110
	uint32_t UnknownHandle114;			// 0x114
	uint32_t UnknownHandle118;			// 0x118
	uint32_t UnknownHandle11C;			// 0x11C

	uint32_t UnknownHandle120;			// 0x120
	uint32_t UnknownHandle124;			// 0x124
	uint32_t UnknownHandle128;			// 0x128
	uint32_t UnknownHandle12C;			// 0x12C

	uint32_t UnknownHandle130;			// 0x130
	uint32_t UnknownFlags134;			// 0x134			
	uint16_t LastUpdateTick;			// 0x138
	std::byte _pad_13A[2];				// 0x13A
	uint32_t UnknownFlags13C;			// 0x13C

	std::byte _pad_140[4];				// 0x140
	uint32_t UnknownFlags144;			// 0x144
	uint32_t UnknownFlags148;			// 0x148
	std::byte _pad_14C[4];				// 0x14C

	uint32_t UnknownHandle150;			// 0x150
	uint32_t UnknownFlags154;			// 0x154
	uint32_t UnknownFlags158;			// 0x158
	uint32_t UnknownFlags15C;			// 0x15C

	uint32_t UnknownFlags160;			// 0x160
	uint32_t UnknownHandle164;			// 0x164
	uint32_t UnknownFlags168;			// 0x168
	uint32_t UnknownFlags16C;			// 0x16C

	std::byte _pad_170[44];				// 0x170

	uint32_t UnknownFlags19C;			// 0x19C

	uint32_t UnknownFlags200;			// 0x200
	uint32_t UnknownHandle204;			// 0x204
	uint32_t UnknownHandle208;			// 0x208
	uint32_t UnknownHandle20C;			// 0x20C

	uint32_t UnknownHandle300;			// 0x304
	uint32_t UnknownHandle304;			// 0x304
	uint32_t UnknownHandle308;			// 0x308
	uint32_t UnknownHandle30C;			// 0x30C

	std::byte _pad_310[8];				// 0x310

	uint32_t UnknownHandle318;			// 0x318
	uint32_t UnknownFlags31C;			// 0x31C

	// ...
};

#pragma pack(pop)

enum class TagType : uint8_t
{
	Invalid = 0xFF,
	Biped = 0x00,
	Vehicle = 0x01,
	Weapons = 0x02,
	Equipment = 0x03,
	Unknown04 = 0x04,
	Projectiles = 0x05,
	SceneryObjects = 0x06,
	DeviceMachine = 0x07,
	Unknown08 = 0x08,
	SoundScenery = 0x09,
	Blocks = 0x0A,
	Unknown0B = 0x0B,
	Unknown0C = 0x0C,
	EffectScenery = 0x0D,
};

struct WeaponObject { };
struct VehicleObject { };
struct BipedObject { };
struct ProjectileObject { };
struct BlockObject { };

// TODO: Where Blam! stores this specific object data?
using SpecificObject = std::variant<std::monostate, 
	WeaponObject, 
	VehicleObject, 
	BipedObject, 
	ProjectileObject, 
	BlockObject>;

// High-level representation of an object inside the ObjectTable.
struct LiveObject
{
	uint32_t DatumIndex{};
	uintptr_t Address{};

	uint32_t Handle{};
	uint32_t NextSiblingHandle{};
	uint32_t ChildHandle{};
	uint32_t ParentHandle{};
	
	std::string Class{};
	std::string TagName{};

	float Position[3]{};
	float Forward[3]{};
	float Up[3]{};
	float LinearVelocity[3]{};
	float AngularVelocity[3]{};

	TagType TagType = TagType::Invalid;
	SpecificObject SpecificObject{};
};