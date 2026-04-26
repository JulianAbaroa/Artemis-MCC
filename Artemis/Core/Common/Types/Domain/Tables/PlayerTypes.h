#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

#pragma pack(push, 1) 

// Raw representation of a player inside the PlayerTable.
struct PlayerHeader
{
	uint32_t Handle;				// 0x000
	uint8_t ConnectedFlag;			// 0x004
	uint8_t UnknownFlag005;         // 0x005
	uint16_t UnknownFlags006;		// 0x006
	uint64_t XUID;                  // 0x008-0x00C

	uint32_t UnknownHandle010;      // 0x010
	uint32_t NetworkId;             // 0x014
	uint16_t Unknown018;            // 0x018
	int16_t  PlayerSessionSlot;     // 0x01A
	int32_t  ControllerIndex;       // 0x01C

	uint32_t Unknown020;            // 0x020
	uint32_t Unknown024;            // 0x024
	uint32_t AliveBipedHandle;		// 0x028
	uint32_t DeadBipedHandle;		// 0x02C

	uint8_t WeaponSyncFlag;			// 0x030
	std::byte _pad_031[3];			// 0x031
	uint32_t CurrentBipedHandle;	// 0x034

	float WeaponPosition[3];		// 0x038-0x03C-0x040
	float WeaponForward[3];			// 0x044-0x048-0x04C
	float AimOffset[3];				// 0x050-0x054-0x058

	uint32_t PrimaryWeaponHandle;	// 0x05C
	uint32_t SecondaryWeaponHandle;	// 0x060
	uint32_t ObjectiveHandle;		// 0x064
	uint32_t UnknownHandle068;		// 0x068
	uint32_t UnknownHandle06C;		// 0x06C
	uint32_t Unknown070;			// 0x070
	uint32_t UnknownHandle074;		// 0x074

	std::byte _pad_078[12];			// 0x078-0x07C-0x080

	uint8_t Unknown084;				// 0x084
	std::byte _pad_085[7];			// 0x085-0x088

	float CameraPosition[3];		// 0x08C-0x090-0x094
	std::byte _pad_098[12];			// 0x098-0x09C-0x100

	uint16_t UnknownFlags;			// 0x104
	std::byte _pad_106[6];			// 0x106-0x108

	uint8_t UnknownFlag;			// 0x10C
	std::byte _pad_10D[3];			// 0x10D

	wchar_t Gamertag[16];			// 0x110-0x114-0x118-0x11C-0x120-0x124-0x128-0x12C

	uint32_t UnknownFlags130;		// 0x130
	uint32_t UnknownFlags134;		// 0x134
	uint32_t UnknownFlags138;		// 0x138
	std::byte _pad_13C[4];			// 0x13C

	uint32_t UnknownFlags140;		// 0x140
	uint32_t UnknownFlags144;		// 0x144
	std::byte _pad_148[12];			// 0x148-0x14C-0x150

	wchar_t Tag[4];					// 0x154-0x158
	std::byte _pad_15C[228];		// 0x15C

	uint32_t UnknownHandle240;		// 0x300
	uint32_t UnknownHandle244;		// 0x304
	std::byte _pad_248[4];			// 0x308

	uint32_t UnknownFlags30C;		// 0x30C

	std::byte _pad_160[672];		// 0x250 | Final padding to match with the 0x490 size.
};

#pragma pack(pop)

static_assert(sizeof(PlayerHeader) == 0x490, "PlayerHeader size mismatch! Expected 0x490");

// High-level representation of a player inside the PlayerTable.
struct LivePlayer
{	
	uint32_t Handle;
	uintptr_t Address;

	uint8_t ConnectedFlag;
	bool IsAlive = false;

	std::string Gamertag;
	std::string Tag;

	float WeaponPosition[3];
	float WeaponForward[3];
	float AimOffset[3];

	uint32_t PrimaryWeaponHandle;
	uint32_t SecondaryWeaponHandle;
	uint32_t ObjectiveHandle;

	uint32_t AliveBipedHandle;
	uint32_t DeadBipedHandle;
	uint32_t CurrentBipedHandle;
};