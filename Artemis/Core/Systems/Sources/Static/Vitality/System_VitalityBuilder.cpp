#include "pch.h"

#include "System_VitalityBuilder.h"

#include "Core/Types/Sources/Static/Vitality/VitalityLayout.h"

#include "Core/States/Sources/MapReader/Tags/State_MapColl.h"
#include "Core/States/Sources/MapReader/Tags/State_MapHlmt.h"
#include "Core/States/Sources/Static/Vitality/State_VitalityBuilder.h"

#include "Core/Systems/Other/Logs/System_Logs.h"

void System_VitalityBuilder::BuildForMap()
{
    int32_t built = 0;
    int32_t skipped = 0;

    const std::vector<std::string> tagNames =
        m_Deps.State_MapHlmt.GetTagNames();

    for (const std::string& tagName : tagNames)
    {
        const HlmtObject* hlmt = 
            m_Deps.State_MapHlmt.Get(tagName);

        if (!hlmt) continue;

        if (hlmt->DamageSections.empty())
        {
            ++skipped;
            continue;
        }

        const CollObject* coll = 
            m_Deps.State_MapColl.Get(tagName);

        VitalityLayout layout = this->BuildLayout(*hlmt, coll);

        if (layout.Sections.empty())
        {
            ++skipped;
            continue;
        }

        m_Deps.State_VitalityBuilder.AddLayout(
            tagName, std::move(layout));

        ++built;
    }

    m_Deps.State_VitalityBuilder.Freeze();

    m_Deps.System_Logs.Log("[VitalityBuilder] INFO: Built."
        " Layouts: %d | Skipped (no damage sections): %d",
        built, skipped);
}

VitalityLayout System_VitalityBuilder::BuildLayout(
    const HlmtObject& hlmt, const CollObject* coll) const
{
    VitalityLayout layout;
    layout.MaximumVitality = hlmt.Data.MaximumVitality;

    if (!hlmt.OldDamageInfo.empty())
    {
        layout.MaximumShieldVitality =
            hlmt.OldDamageInfo[0].MaximumShieldVitality;
    }

    const size_t count = hlmt.DamageSections.size();
    layout.Sections.resize(count);

    for (size_t i = 0; i < count; ++i)
    {
        const Hlmt_DamageSectionsObject& ds = hlmt.DamageSections[i];
        VitalitySectionLayout& out = layout.Sections[i];

        out.NameId = ds.Name;
        out.SectionIndex = static_cast<int>(i);
        out.VitalityPercentage = ds.VitalityPercentage;

        if (ds.ShieldGlobalMaterialName != 0)
        {
            out.Kind = VitalitySectionKind::Shield;

            if (layout.ShieldSection < 0)
            {
                layout.ShieldSection = static_cast<int>(i);
            }
        }

        if (ds.Flags & k_FlagHeadshot)
        {
            out.IsHeadshot = true;
        }

        for (const auto& ir : ds.InstantResponses)
        {
            if ((ir.Flags & k_FlagKillsObject) ||
                (ir.Flags & k_FlagKillsObjectNoSolo))
            {
                out.IsCritical = true;
                break;
            }
        }
        if (out.IsCritical && layout.CriticalSection < 0)
        {
            layout.CriticalSection = static_cast<int>(i);
        }

        if (coll)
        {
            for (size_t r = 0; r < coll->Regions.size(); ++r)
            {
                if (coll->Regions[r].Name == ds.Name)
                {
                    out.CollRegion = static_cast<int>(r);
                    break;
                }
            }
        }
    }

    if (layout.ShieldSection < 0)
    {
        const int idx = hlmt.Data.ShieldedStateDamageSectionIndex;
        if (idx >= 0 && 
            static_cast<size_t>(idx) < layout.Sections.size())
        {
            layout.ShieldSection = idx;
            layout.Sections[idx].Kind = VitalitySectionKind::Shield;
        }
    }

    return layout;
}

void System_VitalityBuilder::Cleanup()
{
    m_Deps.State_VitalityBuilder.Cleanup();

    m_Deps.System_Logs.Log("[VitalityBuilder] INFO:"
        " Cleanup completed.");
}