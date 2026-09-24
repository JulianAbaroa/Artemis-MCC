module Resolved.Vitality.System;

namespace
{
    using Kind = Resolved::Vitality::Type::Kind;
    using Section = Resolved::Vitality::Type::Section;
    using Hlmt_DamageSectionsObject = Map::Tag::Type::Hlmt::Object::Hlmt_DamageSectionsObject;
}

namespace Resolved::Vitality::System
{
    auto VitalityBuilder::BuildForMap() -> void
    {
        std::int32_t built = 0;
        std::int32_t skipped = 0;

        for (const auto& [tagName, hlmt] : m_TagCatalog.Hlmt.All())
        {
            if (hlmt.DamageSections.empty())
            {
                ++skipped;
                continue;
            }

            const CollObject* coll = m_TagCatalog.Coll.Get(tagName);

            ResolvedVitality vitality = this->BuildLayout(hlmt, coll);

            if (vitality.Sections.empty())
            {
                ++skipped;
                continue;
            }

            m_VitalityStore.AddResolvedVitality(tagName, std::move(vitality));

            ++built;
        }

        m_VitalityStore.Freeze();

        m_LogsService.Message("[VitalityBuilder] INFO: Built."
            " Layouts: {} | Skipped (no damage sections): {}",
            built, skipped);
    }

    auto VitalityBuilder::BuildLayout(const HlmtObject& hlmt, const CollObject* coll) const -> ResolvedVitality
    {
        ResolvedVitality vitality;
        vitality.MaximumVitality = hlmt.Data.MaximumVitality;

        if (!hlmt.OldDamageInfo.empty())
        {
            vitality.MaximumShieldVitality =
                hlmt.OldDamageInfo[0].MaximumShieldVitality;
        }

        const std::size_t count = hlmt.DamageSections.size();
        vitality.Sections.resize(count);

        for (std::size_t i = 0; i < count; ++i)
        {
            const Hlmt_DamageSectionsObject& ds = hlmt.DamageSections[i];
            Section& out = vitality.Sections[i];

            out.NameId = ds.Name;
            out.SectionIndex = static_cast<int>(i);
            out.VitalityPercentage = ds.VitalityPercentage;

            if (ds.ShieldGlobalMaterialName != 0)
            {
                out.Kind = Kind::Shield;

                if (vitality.ShieldSection < 0)
                {
                    vitality.ShieldSection = static_cast<int>(i);
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
            if (out.IsCritical && vitality.CriticalSection < 0)
            {
                vitality.CriticalSection = static_cast<int>(i);
            }

            if (coll)
            {
                for (std::size_t r = 0; r < coll->Regions.size(); ++r)
                {
                    if (coll->Regions[r].Name == ds.Name)
                    {
                        out.CollRegion = static_cast<int>(r);
                        break;
                    }
                }
            }
        }

        if (vitality.ShieldSection < 0)
        {
            const int idx = hlmt.Data.ShieldedStateDamageSectionIndex;
            if (idx >= 0 &&
                static_cast<std::size_t>(idx) < vitality.Sections.size())
            {
                vitality.ShieldSection = idx;
                vitality.Sections[idx].Kind = Kind::Shield;
            }
        }

        return vitality;
    }

    auto VitalityBuilder::Cleanup() -> void
    {
        m_VitalityStore.Cleanup();

        m_LogsService.Message("[VitalityBuilder] INFO: Cleanup completed.");
    }
}