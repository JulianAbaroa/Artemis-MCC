export module Tables.Object.Type:DamageSection;

import std;

export namespace Tables::Object::Type::DamageSection
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

    struct DamageSectionTable
    {
        std::uintptr_t BaseAddress{};
        std::vector<DamageSectionState> Sections{};
    };
}