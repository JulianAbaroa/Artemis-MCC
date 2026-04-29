#pragma once

#include <cstdint>
#include <string>

#pragma pack(push, 1)

struct Map_SBSPInstancedGeometry
{
	int32_t NameStringID;
};

struct Map_SBSP
{
	int32_t NumberOfInstancedGeometry;
	uint32_t InstancedGeometryTableAddress;
};

// Base: 0x2C, Count: 12, EntrySize: 0x4.
struct Map_Unic_LanguageRanges
{
	int16_t RangeStartIndex;
	int16_t RangeSize;
};

struct Map_Unic
{
	Map_Unic_LanguageRanges LanguageRanges;
};

struct Map_DataRef
{
	int32_t Size;				// 0x00
	std::byte _pad_0x04[8];		// 0x04
	uint32_t Pointer;			// 0x0C
	std::byte _pad_0x10[4];		// 0x10
};

struct Map_TagRef
{
	int32_t TagGroupMagic;
	std::byte _pad_0x04[8];
	uint32_t DatumIndex;
};

// Size: 0xC.
struct Map_TagBlock
{
	int32_t EntryCount;
	uint32_t Pointer;
	std::byte _pad_0x08[4];
};

// Size: 0x8.
struct Map_LocaleIndexTableElement
{
	uint32_t StringID;
	uint32_t Offset;
};

// Base: 0x18, Count: 12, EntrySize: 0x50.
struct Map_MapGlobalMeta_Languages
{
	uint32_t StringCount;
	uint32_t LocaleTableSize;
	uint32_t LocaleIndexTableOffset;
	uint32_t LocaleDataIndexOffset;

	std::byte IndexTableHash[14];
	std::byte StringDataHash[14];
};

struct Map_MapGlobalMeta
{
	Map_MapGlobalMeta_Languages Languages;
};

// Size: 0x8.
struct Map_TagInteropTableEntry
{
	uint32_t Pointer;
	int32_t Type;
};

// Size: 0x8.
struct Map_GlobalTagTableEntry
{
	int32_t TagGroupMagic;
	uint32_t DatumIndex;
};

// Size: 0x8.
struct Map_TagTableEntry
{
	int16_t TagGroupIndex;
	uint16_t DatumIndexSalt;
	uint32_t MemoryAddress;
};

// Size: 0x10.
struct Map_TagTableGroupEntry
{
	int32_t Magic;
	int32_t ParentMagic;
	int32_t GranParentMagic;
	int32_t StringID;
};

// Size: 0x4C.
struct Map_IndexTableHeader
{
	int32_t NumberOfTagGroups;
	uint64_t TagGroupTableAddress;
	int32_t NumberOfTags;
	uint64_t TagTableAddress;
	int32_t NumberOfGlobalTags;
	uint64_t GlobalTagTableAddress;
	int32_t NumberOfTagInterops;
	uint64_t TagInteropTableAddress;
	int32_t Magic;
};

// Base: 0x4DC, Count: 4, EntrySize: 0x8.
struct Map_Header_Sections
{
	uint32_t VirtualAddress;
	uint32_t Size;
};

// Base: 0x4CC, Count: 4, EntrySize: 0x4.
struct Map_Header_OffsetMasks
{
	uint32_t Mask;
};

// Base: 0x300, Count: 6, EntrySize: 0x10.
struct Map_Header_Partitions
{
	uint64_t LoadAddress;
	uint64_t Size;
};

// Size: 0xA000.
struct Map_Header
{
	uint32_t FileSize;
	uint32_t TagBufferOffset;
	uint32_t VirtualSize;

	int16_t Type;

	int32_t FileTableCount;
	int32_t FileTableOffset;
	int32_t FileTableSize;
	int32_t FileIndexTableOffset;

	int32_t StringTableCount;
	int32_t StringTableOffset;
	int32_t StringTableSize;
	int32_t StringIndexTableOffset;
	int32_t StringNamespaceTableCount;
	int32_t StringNamespaceTableOffset;

	int32_t CacheBuildDateHigh;
	int32_t CacheBuildDateLow;

	char BuildString[32];

	char InternalName[32];
	char ScenarioName[64];

	int32_t CheckSum;

	uint64_t VirtualBaseAddress;
	uint64_t IndexHeaderAddress;
	uint32_t XDKVersion;

	Map_Header_Partitions Partitions;
	Map_Header_OffsetMasks OffsetMasks;
	Map_Header_Sections Sections;
};

// Common.

struct Vec2 { float X, Y; };
struct Vec3 { float X, Y, Z; };
struct Vec4 { float X, Y, Z, W; };

struct ColorRGB { float Red, Green, Blue; };
struct ColorRGBA { float Red, Green, Blue, Aplha; };

struct RangeF { float Min, Max; };
struct RangeD { double Min, Max; };

#pragma pack(pop)

struct TagInfo
{
	bool IsValid = false;
	std::string ClassName;
	std::string TagName;
};