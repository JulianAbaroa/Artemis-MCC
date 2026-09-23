export module Map.Reader.Type:Structure;

import std;

export namespace Map::Reader::Type::Structure
{
#pragma pack(push, 1)
	namespace Header
	{
		struct IndexTable
		{
			std::int32_t NumberOfTagGroups{};
			std::uint64_t TagGroupTableAddress{};
			std::int32_t NumberOfTags{};
			std::uint64_t TagTableAddress{};
			std::int32_t NumberOfGlobalTags{};
			std::uint64_t GlobalTagTableAddress{};
			std::int32_t NumberOfTagInterops{};
			std::uint64_t TagInteropTableAddress{};
			std::int32_t Magic{};
		};

		struct Section
		{
			std::uint32_t VirtualAddress{};
			std::uint32_t Size{};
		};

		struct OffsetMask
		{
			std::uint32_t Mask{};
		};

		struct Partition
		{
			std::uint64_t LoadAddress{};
			std::uint64_t Size{};
		};

		struct Header
		{
			std::uint32_t FileSize{};
			std::uint32_t TagBufferOffset{};
			std::uint32_t VirtualSize{};

			std::int16_t Type{};

			std::int32_t FileTableCount{};
			std::int32_t FileTableOffset{};
			std::int32_t FileTableSize{};
			std::int32_t FileIndexTableOffset{};

			std::int32_t StringTableCount{};
			std::int32_t StringTableOffset{};
			std::int32_t StringTableSize{};
			std::int32_t StringIndexTableOffset{};
			std::int32_t StringNamespaceTableCount{};
			std::int32_t StringNamespaceTableOffset{};

			std::int32_t CacheBuildDateHigh{};
			std::int32_t CacheBuildDateLow{};

			char BuildString[32]{};

			char InternalName[32]{};
			char ScenarioName[64]{};

			std::int32_t CheckSum{};

			std::uint64_t VirtualBaseAddress{};
			std::uint64_t IndexHeaderAddress{};
			std::uint32_t XDKVersion{};

			Partition Partitions{};
			OffsetMask OffsetMasks{};
			Section Sections{};
		};
	}

	namespace Tag
	{
		struct Reference
		{
			std::int32_t TagGroupMagic{};
			std::byte _pad_0x04[8];
			std::uint32_t DatumIndex{};
		};

		struct Block
		{
			std::int32_t EntryCount{};
			std::uint32_t Pointer{};
			std::byte _pad_0x08[4];
		};

		struct TagInteropTableEntry
		{
			std::uint32_t Pointer{};
			std::int32_t Type{};
		};
	}

	namespace TagTable
	{
		struct Entry
		{
			std::int16_t TagGroupIndex{};
			std::uint16_t DatumIndexSalt{};
			std::uint32_t MemoryAddress{};
		};

		struct GroupEntry
		{
			std::int32_t Magic{};
			std::int32_t ParentMagic{};
			std::int32_t GranParentMagic{};
			std::int32_t StringID{};
		};

		struct GlobalEntry
		{
			std::int32_t TagGroupMagic{};
			std::uint32_t DatumIndex{};
		};
	}

	namespace Data
	{
		struct Reference
		{
			std::int32_t Size{};
			std::byte _pad_0x04[8];
			std::uint32_t Pointer{};
			std::byte _pad_0x10[4];
		};
	}

	namespace Sbsp
	{
		struct InstancedGeometry
		{
			std::int32_t NameStringID{};
		};

		struct Sbsp
		{
			std::int32_t NumberOfInstancedGeometry{};
			std::uint32_t InstancedGeometryTableAddress{};
		};
	}

	namespace Unic
	{
		struct LanguageRange
		{
			std::int16_t RangeStartIndex{};
			std::int16_t RangeSize{};
		};

		struct Unic
		{
			LanguageRange LanguageRanges{};
		};
	}

	namespace Localization
	{
		struct IndexTableElement
		{
			std::uint32_t StringID{};
			std::uint32_t Offset{};
		};

		struct Languages
		{
			std::uint32_t StringCount{};
			std::uint32_t LocaleTableSize{};
			std::uint32_t LocaleIndexTableOffset{};
			std::uint32_t LocaleDataIndexOffset{};

			std::byte IndexTableHash[14]{};
			std::byte StringDataHash[14]{};
		};

		struct MapGlobalMeta
		{
			Languages Languages{};
		};
	}

	namespace Primitive
	{
		struct Vec2 { float X{}, Y{}; };
		struct Vec3 { float X{}, Y{}, Z{}; };
		struct Vec4 { float X{}, Y{}, Z{}, W{}; };

		struct ColorRGB { float Red{}, Green{}, Blue{}; };
		struct ColorRGBA { float Red{}, Green{}, Blue{}, Alpha{}; };

		struct RangeF { float Min{}, Max{}; };
		struct RangeD { double Min{}, Max{}; };
	}
#pragma pack(pop)
}