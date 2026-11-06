export module Blam.Runtime.Object.Type:DamageSection;

import std;

export namespace Blam::Runtime::Object::Type::DamageSection
{
#pragma pack(push, 1)
    struct DamageSection
    {
        std::uint8_t DamageLevelMask{};
        std::uint8_t _pad_01[0x0F];
        float Vitality{};
        std::int32_t Sentinel{};
    };
    static_assert(sizeof(DamageSection) == 0x18);
#pragma pack(pop)

    struct DamageSectionState
    {
        std::uint8_t DamageLevelMask{};
        float Vitality{};
    };

    // Raw block with the damage state of each region, as the engine keeps it in memory.
    // note: Located by a size and offset pair in the object header, next to the damage section one.
    struct RegionBlock
    {
        std::uint16_t Size{};
        std::uint16_t Offset{};

        std::vector<std::uint8_t> Bytes{};
    };

    struct DamageSectionTable
    {
        std::vector<DamageSectionState> Sections{};

        RegionBlock Regions{};
    };

    // The table of an object and the Clear generation of the store it was written in (0 = never, or discarded).
    struct DamageSectionEntry
    {
        DamageSectionTable Table{};
        std::uint64_t Generation{};
    };

    // Damage section tables, by object handle. The entries of older generations are kept for their buffers and are not visible.
    struct DamageSectionTables : std::unordered_map<std::uint32_t, DamageSectionEntry>
    {
        using unordered_map::unordered_map;
    };
}