#pragma once

#include <cstdint>
#include <vector>

enum class VitalitySectionKind : uint8_t
{
    Normal,
    Shield,
};

struct VitalitySectionLayout
{
    uint32_t NameId = 0;

    int SectionIndex = -1;
    int CollRegion = -1;

    float VitalityPercentage = 0.0f;

    bool IsCritical = false;
    bool IsHeadshot = false;

    VitalitySectionKind Kind = VitalitySectionKind::Normal;
};

struct VitalityLayout
{
    std::vector<VitalitySectionLayout> Sections;

    int CriticalSection = -1;
    int ShieldSection = -1;

    float MaximumVitality = 0.0f;
    float MaximumShieldVitality = 0.0f;
};