export module Resolved.Vitality.Type;

import std;

export namespace Resolved::Vitality::Type
{
    enum class Kind : std::uint8_t
    {
        Normal,
        Shield,
    };

    struct Section
    {
        std::uint32_t NameId = 0;

        int SectionIndex = -1;
        int CollRegion = -1;

        float VitalityPercentage = 0.0f;

        bool IsCritical = false;
        bool IsHeadshot = false;

        Kind Kind = Kind::Normal;
    };

    struct Vitality
    {
        std::vector<Section> Sections;

        int CriticalSection = -1;
        int ShieldSection = -1;

        float MaximumVitality = 0.0f;
        float MaximumShieldVitality = 0.0f;
    };
}