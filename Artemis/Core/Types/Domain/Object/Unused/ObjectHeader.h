#pragma once

#include <cstdint>
#include <cstddef>

#pragma pack(push, 1)

// Raw representation of an object inside the ObjectTable 
struct ObjectHeader
{
	uint32_t DatumIndex;				// 0x000
	std::byte _pad_004[8];				// 0x004
	uint32_t NextSiblingHandle;			// 0x00C

	uint32_t ChildHandle;				// 0x010		
	uint32_t ParentHandle;				// 0x014
	std::byte _pad_018[8];				// 0x018

	float CurrentPosition[3];			// 0x020-0x028
	float CurrentRadius;				// 0x02C

	float LastNotifiedPosition[3];		// 0x030-0x038
	float LastNotifiedRadius;			// 0x03C

	std::byte _pad_040[4];				// 0x040
	float PhysicsPosition[3];			// 0x044-0x04C

	float Forward[3];					// 0x050-0x058
	float Up[3];						// 0x05C-0x064

	float LinearVelocity[3];			// 0x068-0x070
	float AngularVelocity[3];			// 0x074-0x07C

	std::byte _pad_080[16];				// 0x080
	std::byte _pad_090[16];             // 0x090
	std::byte _pad_0A0[16];             // 0x0A0
	std::byte _pad_0B0[16];             // 0x0B0
	std::byte _pad_0C0[16];             // 0x0C0
	std::byte _pad_0D0[16];             // 0x0D0
	std::byte _pad_0E0[16];             // 0x0E0
	std::byte _pad_0F0[16];             // 0x0F0
	std::byte _pad_100[16];             // 0x100
	std::byte _pad_110[16];             // 0x110
	std::byte _pad_120[16];             // 0x120
	std::byte _pad_130[16];             // 0x130
	std::byte _pad_140[16];             // 0x140
	std::byte _pad_150[16];             // 0x150
	std::byte _pad_160[16];             // 0x160
	std::byte _pad_170[16];             // 0x170
	std::byte _pad_180[16];             // 0x180
	std::byte _pad_190[16];             // 0x190
	std::byte _pad_1A0[16];             // 0x1A0
	std::byte _pad_1B0[16];             // 0x1B0
	
	std::byte _pad_1C0[10];             // 0x1C0
	uint8_t ActionState;				// 0x1CA
	uint8_t Team;						// 0x1CB
	std::byte _pad_1CC[4];				// 0x1CC

	std::byte _pad_1D0[16];				// 0x1D0

	float TotalHeat;					// 0x1E0
	float TotalEnergy;					// 0x1E4
	std::byte _pad_1E8[8];				// 0x1E8 

	std::byte _pad_1F0[16];             // 0x1F0 
	std::byte _pad_200[16];             // 0x200 
	std::byte _pad_210[16];             // 0x210 
	std::byte _pad_220[16];             // 0x220 
	std::byte _pad_230[16];             // 0x230 
	std::byte _pad_240[16];             // 0x240 
	std::byte _pad_250[16];             // 0x250 
	std::byte _pad_260[16];             // 0x260 
	std::byte _pad_270[16];             // 0x270 
	std::byte _pad_280[16];             // 0x280 
	std::byte _pad_290[16];             // 0x290 
	std::byte _pad_2A0[16];             // 0x2A0 
	std::byte _pad_2B0[16];				// 0x2B0 

	uint8_t IsReloading;				// 0x2C0
	std::byte _pad_2C1[5];				// 0x2C1
	uint16_t TotalAmmo;					// 0x2C6
	std::byte _pad_2C7[2];				// 0x2C7
	uint16_t CurrentAmmo;				// 0x2CA
	std::byte _pad_2CB[4];				// 0x2CB

	std::byte _pad_2D0[16];				// 0x2D0
	std::byte _pad_2E0[16];				// 0x2E0
	std::byte _pad_2F0[16];				// 0x2F0
	std::byte _pad_300[16];				// 0x300
	std::byte _pad_310[16];				// 0x310
	std::byte _pad_320[16];				// 0x320
};
static_assert(sizeof(ObjectHeader) == 0x330, "ObjectHeader size mismatch! Expected 0x330");

#pragma pack(pop)