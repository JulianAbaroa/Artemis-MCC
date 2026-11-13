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
    // note: The engine calls the block "game globals": mode and game variant (the variant at +0x14E8, FUN_18007a164).
    constexpr std::uintptr_t k_ThreadContextGameModeOffset{ 0x48 };
    constexpr std::uintptr_t k_GameModeTypeOffset{ 0x10 };

    // Bytes the engine registers for the game globals (FUN_180056a88), copied whole into the snapshot (D249).
    constexpr std::size_t k_GameGlobalsSize{ 0x1F0E0 };

    // Game mode with a map variant (multiplayer, Forge included).
    constexpr std::uint8_t k_GameModeMultiplayer{ 3 };

    // Pointer of the thread context that leads to the map variant.
    // note: mapVariant = (pointer + k_MapVariantAdjust) & ~k_MapVariantAlignMask, as the engine computes it (FUN_18006bcec).
    constexpr std::uintptr_t k_ThreadContextMapVariantOffset{ 0x290 };
    constexpr std::uintptr_t k_MapVariantAdjust{ 0x30B };
    constexpr std::uintptr_t k_MapVariantAlignMask{ 0x3 };

    // Bytes of the map variant (unpacked), and the places it is copied from or to (Q98, D266, A15 sec. 5, MapVariant sec. 7.1).
    // note: The match configuration of MCC holds it at k_ConfigurationMapVariantOffset (it ends at the end of the configuration data),
    // the game globals hold the copy the engine loads from at k_GameGlobalsMapVariant (game options + 0x110E0, unaligned).
    constexpr std::size_t k_MapVariantSize{ 0xD9BC };
    constexpr std::size_t k_ConfigurationMapVariantOffset{ 0x1D308 };
    constexpr std::size_t k_GameGlobalsMapVariant{ 0x110F0 };

    // Map id of Forge World, the only map whose map variants Artemis builds configurations from (Q106, D290).
    constexpr std::uint32_t k_ForgeWorldMapId{ 3006 };

    // Version the engine accepts in the map variant header (Validate).
    constexpr std::uint16_t k_MapVariantVersion{ 0x20 };

    // Header of the map variant, from its first byte: version, number of quotas (palette entries), id of the map it was made for, and the
    // world bounds the engine recomputes at load (FUN_18006c290) and Q98 keeps from the runtime variant.
    constexpr std::size_t k_MapVariantVersionOffset{ 0x2B0 };
    constexpr std::size_t k_MapVariantQuotasOffset{ 0x2B2 };
    constexpr std::size_t k_MapVariantMapIdOffset{ 0x2B4 };
    constexpr std::size_t k_MapVariantBoundsOffset{ 0x2C8 };
    constexpr std::size_t k_MapVariantBoundsSize{ 0x18 };

    // Label table of the map variant (FUN_18014daf0 registers its labels in the first round after the load): the strings, zero terminated,
    // in an area of k_MapVariantLabelStringsSize bytes, the offset of string i inside that area as a u16 in an array of k_MaxLabels entries,
    // and the number of labels as an i32. A placement points to its label with k_GametypeLabel (Mvar), k_NoLabel when it has none.
    // note: The per-label object lists of Megalo (megalo state + 0x13A0 + label * 4) are indexed by the label index of the load: a map
    // variant written between rounds has to keep the index of every label the match already knows (RemapLabels).
    constexpr std::size_t k_MapVariantLabelStringsOffset{ 0x2F4 };
    constexpr std::size_t k_MapVariantLabelStringsSize{ 0x1000 };
    constexpr std::size_t k_MapVariantLabelOffsetsOffset{ 0x12F8 };
    constexpr std::size_t k_MaxLabels{ 256 };
    constexpr std::size_t k_MapVariantLabelCountOffset{ 0x14F8 };
    constexpr std::uint16_t k_NoLabel{ (std::numeric_limits<std::uint16_t>::max)() };

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

    // Bytes the engine registers for the player mapping globals (FUN_180043440), copied whole into the snapshot (D249).
    constexpr std::size_t k_MappingGlobalsSize{ 0xE8 };

    // Pointer of the thread context to the player control globals, and the output user (view) entries inside them (A9 sec. 11.2).
    constexpr std::uintptr_t k_ThreadContextControlGlobalsOffset{ 0xD8 };
    constexpr std::uintptr_t k_ControlGlobalsOutputUserTableOffset{ 0x680 };

    // Pointer of the thread context to the game engine globals: phase and scores of the match (D103).
    constexpr std::uintptr_t k_ThreadContextGameEngineOffset{ 0x20 };

    // Bytes the engine registers for the game engine globals (FUN_18007853c), copied whole into the snapshot (D249).
    constexpr std::size_t k_GameEngineGlobalsSize{ 0x8430 };

    // Pointer of the thread context to the time globals, and their fields: engine tick, speed of the engine (D105), tick fraction.
    constexpr std::uintptr_t k_ThreadContextTimeGlobalsOffset{ 0xA0 };
    constexpr std::uintptr_t k_TimeGlobalsEngineTickOffset{ 0x0C };
    constexpr std::uintptr_t k_TimeGlobalsScaleOffset{ 0x10 };
    constexpr std::uintptr_t k_TimeGlobalsTickFractionOffset{ 0x14 };

    // Bytes the engine registers for the time globals (FUN_18005c218), copied whole into the snapshot (D249).
    constexpr std::size_t k_TimeGlobalsSize{ 0x3C };

    // Pointer of the thread context to the event globals, and the last sequence of the event ring inside them (D104).
    constexpr std::uintptr_t k_ThreadContextEventGlobalsOffset{ 0x98 };
    constexpr std::uintptr_t k_EventGlobalsSequenceOffset{ 0x10B30 };

    // Bytes the engine registers for the event globals ("incident globals", FUN_1801092b0), copied whole into the snapshot (D249).
    constexpr std::size_t k_EventGlobalsSize{ 0x10B68 };

    // Data array of the engine (ResourceBuffer_Allocate): maximum count (int32), element size (int32) and first element (pointer).
    // note: +0x44 is the first slot never used, not the capacity: free slots above it are still valid (datum_new_at_index), but every
    // live datum is below it (BlamDataArrayAllocateAtIndex raises it to index + 1). +0x48 is the count of live datums.
    constexpr std::uintptr_t k_DataArrayCapacity{ 0x2C };
    constexpr std::uintptr_t k_DataArrayFirstUnusedIndex{ 0x44 };
    constexpr std::uintptr_t k_DataArrayLiveCount{ 0x48 };
    constexpr std::uintptr_t k_DataArrayElementSize{ 0x20 };
    constexpr std::uintptr_t k_DataArrayData{ 0x50 };

    // Pointer of the thread context to the "megalo_objects" data array (FUN_18007853c): the state the gametype script keeps per
    // object (timers and variables of Megalo), its element index in the object datum +0x158 (D250).
    constexpr std::uintptr_t k_ThreadContextMegaloObjectsOffset{ 0x4F0 };

    // Elements of the megalo objects data array and bytes of each, all copied into the snapshot (D250).
    constexpr std::size_t k_MaxMegaloObjects{ 0x200 };
    constexpr std::size_t k_MegaloObjectSize{ 0x94 };
}