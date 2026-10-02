export module Resolved.Definitions.Reflect:Hlmt;

import Common.Reflect.Type;
import Resolved.Definitions.Type;
import std;

namespace
{
    using InstantResponse = Resolved::Definitions::Type::Hlmt::InstantResponse;
    using DamageSection = Resolved::Definitions::Type::Hlmt::DamageSection;
    using Hlmt = Resolved::Definitions::Type::Hlmt::Hlmt;
}

export namespace Common::Reflect::Type
{
    template <>
    struct Fields<InstantResponse>
    {
        static constexpr bool HasFields = true;
        static constexpr auto Value = std::tuple{
            MakeField("Flags", &InstantResponse::Flags),
        };
    };

    template <>
    struct Fields<DamageSection>
    {
        static constexpr bool HasFields = true;
        static constexpr auto Value = std::tuple{
            MakeField("Name", &DamageSection::Name),
            MakeField("Flags", &DamageSection::Flags),
            MakeField("VitalityPercentage", &DamageSection::VitalityPercentage),
            MakeField("ShieldGlobalMaterialName", &DamageSection::ShieldGlobalMaterialName),
            MakeField("InstantResponses", &DamageSection::InstantResponses),
        };
    };

    template <>
    struct Fields<Hlmt>
    {
        static constexpr bool HasFields = true;
        static constexpr auto Value = std::tuple{
            MakeField("TagName", &Hlmt::TagName),
            MakeField("MaximumVitality", &Hlmt::MaximumVitality),
            MakeField("MaximumShieldVitality", &Hlmt::MaximumShieldVitality),
            MakeField("ShieldedStateDamageSectionIndex", &Hlmt::ShieldedStateDamageSectionIndex),
            MakeField("DamageSections", &Hlmt::DamageSections),
        };
    };
}