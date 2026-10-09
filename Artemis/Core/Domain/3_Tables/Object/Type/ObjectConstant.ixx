export module Tables.Object.Type:Constant;

import std;

export namespace Tables::Object::Type::Constant
{
	constexpr std::uint8_t k_DamageSectionStride{ 0x18 };
	constexpr std::uint16_t k_RegionBlockMaxSize{ 0x400 };

	// Slots the object table is scanned for.
	// note: Assumed from the object limit of the engine. Not read from it.
	constexpr std::uint32_t k_MaxObjects{ 1024 };

	// Bytes of one entry of the object table.
	constexpr std::uintptr_t k_EntrySize{ 0x18 };

	// Offsets inside an entry of the object table.
	constexpr std::uintptr_t k_EntrySaltOffset{ 0x00 };
	constexpr std::uintptr_t k_EntryKindOffset{ 0x04 };
	constexpr std::uintptr_t k_EntryAddressOffset{ 0x10 };
}