export module Template.Object.Type:Constant;

import std;

export namespace Template::Object::Type::Constant
{
    // Slots the object table is scanned for.
    // note: Assumed from the object limit of the engine. Not read from it.
    constexpr std::uint32_t k_MaxObjects{ 1024 };

    // Bytes of one entry of the object table.
    constexpr std::uintptr_t k_EntrySize{ 0x18 };

    // Offsets inside an entry of the object table.
    constexpr std::uintptr_t k_EntrySaltOffset{ 0x00 };
    constexpr std::uintptr_t k_EntryKindOffset{ 0x04 };
    constexpr std::uintptr_t k_EntryDatumSizeOffset{ 0x06 };
    constexpr std::uintptr_t k_EntryAddressOffset{ 0x10 };

    // Smallest datum size an entry can report: the base object component.
    // note: Entries below it are skipped as corrupt.
    constexpr std::uint16_t k_MinDatumSize{ 0x1A8 };
}