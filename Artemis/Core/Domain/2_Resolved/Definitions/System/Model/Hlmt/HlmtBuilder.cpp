module Resolved.Definitions.System;
import :Hlmt;

import std;

namespace Resolved::Definitions::System
{
    auto HlmtBuilder::Build(const HlmtObject& hlmt) -> ResolvedHlmt
    {
        ResolvedHlmt out{};

        out.TagName = hlmt.TagName;

        out.MaximumVitality = hlmt.Data.MaximumVitality;
        out.ShieldedStateDamageSectionIndex = hlmt.Data.ShieldedStateDamageSectionIndex;

        if (!hlmt.OldDamageInfo.empty())
        {
            out.MaximumShieldVitality = hlmt.OldDamageInfo[0].MaximumShieldVitality;
        }

        out.DamageSections.reserve(hlmt.DamageSections.size());

        for (const auto& ds : hlmt.DamageSections)
        {
            DamageSection& section = out.DamageSections.emplace_back();

            section.Name = ds.Name;
            section.Flags = ds.Flags;
            section.VitalityPercentage = ds.VitalityPercentage;
            section.ShieldGlobalMaterialName = ds.ShieldGlobalMaterialName;

            section.InstantResponses.reserve(ds.InstantResponses.size());

            for (const auto& ir : ds.InstantResponses)
            {
                section.InstantResponses.push_back(InstantResponse{ .Flags = ir.Flags });
            }
        }

        return out;
    }
}