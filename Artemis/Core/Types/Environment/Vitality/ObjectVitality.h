#pragma once

#include "Core/Types/Sources/Static/Vitality/VitalityLayout.h"
#include <cstdint>

struct VitalitySection
{
    uint32_t NameId = 0;
    float Vitality = 0.0f;

    int CollRegion = -1;
    bool IsCritical = false;
    bool IsHeadshot = false;
    VitalitySectionKind Kind = VitalitySectionKind::Normal;
};

struct ObjectVitality
{
    uint32_t Handle = 0;

    std::vector<VitalitySection> Sections;

    int CriticalSection = -1;
    int ShieldSection = -1;

    bool IsDead = false;
};