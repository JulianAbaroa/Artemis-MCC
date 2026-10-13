export module Datum.Object.Type:Constant;

import std;

export namespace Datum::Object::Type::Constant
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

    // Pointer of the thread context to the game mode, and the offset of the mode inside it.
    constexpr std::uintptr_t k_ThreadContextGameModeOffset{ 0x48 };
    constexpr std::uintptr_t k_GameModeTypeOffset{ 0x10 };

    // Game mode with a map variant (multiplayer, Forge included).
    constexpr std::uint8_t k_GameModeMultiplayer{ 3 };

    // Pointer of the thread context that leads to the map variant.
    // note: mapVariant = (pointer + k_MapVariantAdjust) & ~k_MapVariantAlignMask, as the engine computes it (FUN_18006bcec).
    constexpr std::uintptr_t k_ThreadContextMapVariantOffset{ 0x290 };
    constexpr std::uintptr_t k_MapVariantAdjust{ 0x30B };
    constexpr std::uintptr_t k_MapVariantAlignMask{ 0x3 };

    // Placement array of the map variant: one entry per object placed in Forge.
    // note: The layout of an entry is Mvar (Data/Datum/mvar.xml). The limit is the Forge object limit.
    constexpr std::uintptr_t k_PlacementsOffset{ 0x14FC };
    constexpr std::uint32_t k_MaxPlacements{ 0x28B };
    constexpr std::uintptr_t k_PlacementSize{ 0x4C };
}