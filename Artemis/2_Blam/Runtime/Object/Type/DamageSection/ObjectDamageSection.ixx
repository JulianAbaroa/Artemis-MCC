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

    // The table of an object and whether the store shows it (an invisible entry is kept for its buffers).
    struct DamageSectionEntry
    {
        DamageSectionTable Table{};
        bool IsVisible{};
    };

    // Damage section tables, by object handle. The invisible entries are kept for their buffers until the store prunes them.
    struct DamageSectionTables : std::unordered_map<std::uint32_t, DamageSectionEntry>
    {
        using unordered_map::unordered_map;
    };
}