export module Blam.Layout.Object.Type:Constant;

import std;

export namespace Blam::Layout::Object::Type::Constant
{
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

    // Pointer of the thread context to the object data, and the object table inside it.
    constexpr std::uintptr_t k_ThreadContextObjectDataOffset{ 0x10 };
    constexpr std::uintptr_t k_ObjectDataTableOffset{ 0x50 };

    // Pointer of the thread context to the telemetry data, and the player table inside it.
    constexpr std::uintptr_t k_ThreadContextTelemetryDataOffset{ 0x18 };
    constexpr std::uintptr_t k_TelemetryDataPlayerTableOffset{ 0x50 };

    // Pointer of the thread context to the player mapping globals (A9 sec. 11.1).
    constexpr std::uintptr_t k_ThreadContextMappingOffset{ 0x158 };

    // Pointer of the thread context to the player control globals, and the output user (view) entries inside them (A9 sec. 11.2).
    constexpr std::uintptr_t k_ThreadContextControlGlobalsOffset{ 0xD8 };
    constexpr std::uintptr_t k_ControlGlobalsOutputUserTableOffset{ 0x680 };

    // Pointer of the thread context to the game engine globals: phase and scores of the match (D103).
    constexpr std::uintptr_t k_ThreadContextGameEngineOffset{ 0x20 };

    // Pointer of the thread context to the time globals, and their fields: engine tick, speed of the engine (D105), tick fraction.
    constexpr std::uintptr_t k_ThreadContextTimeGlobalsOffset{ 0xA0 };
    constexpr std::uintptr_t k_TimeGlobalsEngineTickOffset{ 0x0C };
    constexpr std::uintptr_t k_TimeGlobalsScaleOffset{ 0x10 };
    constexpr std::uintptr_t k_TimeGlobalsTickFractionOffset{ 0x14 };

    // Pointer of the thread context to the event globals, and the last sequence of the event ring inside them (D104).
    constexpr std::uintptr_t k_ThreadContextEventGlobalsOffset{ 0x98 };
    constexpr std::uintptr_t k_EventGlobalsSequenceOffset{ 0x10B30 };
}