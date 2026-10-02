export module Resolved.Definitions.Type:Hlmt;

import std;

export namespace Resolved::Definitions::Type::Hlmt
{
    struct InstantResponse
    {
        std::uint32_t Flags{};
    };

    struct DamageSection
    {
        std::uint32_t Name{};
        std::uint32_t Flags{};

        float VitalityPercentage{};

        // Non-zero when this section is backed by a shield material
        std::uint32_t ShieldGlobalMaterialName{};

        std::vector<InstantResponse> InstantResponses{};
    };

    struct Hlmt
    {
        std::string TagName{};

        float MaximumVitality{};
        float MaximumShieldVitality{};

        std::int32_t ShieldedStateDamageSectionIndex{ -1 };

        std::vector<DamageSection> DamageSections{};
    };
}