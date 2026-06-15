#pragma once

#include <cstdint>
#include <vector>

#pragma pack(push, 1)
struct DamageSection
{
    uint8_t DamageLevelMask;
    uint8_t _pad_01[0x0F];
    float Vitality;
    int32_t Sentinel;
};
static_assert(sizeof(DamageSection) == 0x18);
#pragma pack(pop)

struct DamageSectionState
{
    uint8_t DamageLevelMask;
    float Vitality;
};

struct DamageSectionTable
{
    uintptr_t BaseAddress;
    std::vector<DamageSectionState> Sections;
};