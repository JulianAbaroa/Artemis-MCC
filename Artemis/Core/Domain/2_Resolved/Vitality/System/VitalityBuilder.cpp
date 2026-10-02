module Resolved.Vitality.System;

import Resolved.World.Type;

namespace
{
    using Kind = Resolved::Vitality::Type::Vitality::Kind;
    using Transfer = Resolved::Vitality::Type::Vitality::Transfer;
    using AnchorSource = Resolved::World::Type::ModelLink::AnchorSource;
    using DamageSection = Resolved::Definitions::Type::Hlmt::DamageSection;

    using Resolved::Vitality::Type::Constant::k_FlagKillsObject;
    using Resolved::Vitality::Type::Constant::k_FlagKillsObjectNoSolo;
    using Resolved::Vitality::Type::Constant::k_FlagDestroysObject;
    using Resolved::Vitality::Type::Constant::k_FlagHeadshot;
}

namespace Resolved::Vitality::System
{
    auto VitalityBuilder::BuildForMap() -> void
    {
        std::int32_t built = 0;
        std::int32_t skipped = 0;
        std::int32_t killSections = 0;
        std::int32_t headshotSections = 0;
        std::int32_t headshotAnchored = 0;

        for (const auto& [tagName, hlmt] : m_DefinitionsStore.GetAllResolvedHlmts())
        {
            if (hlmt.DamageSections.empty())
            {
                ++skipped;
                continue;
            }

            const ResolvedModelLink* link = m_WorldStore.GetResolvedModelLink(tagName);

            ResolvedVitality vitality = this->BuildLayout(hlmt, link);

            if (vitality.Sections.empty())
            {
                ++skipped;
                continue;
            }

            killSections += static_cast<std::int32_t>(vitality.KillSections.size());
            headshotSections += static_cast<std::int32_t>(vitality.HeadshotSections.size());
            for (const int index : vitality.HeadshotSections)
            {
                if (vitality.Sections[index].Aim.Source != AnchorSource::None &&
                    vitality.Sections[index].Aim.Source != AnchorSource::ObjectCenter)
                {
                    ++headshotAnchored;
                }
            }

            m_VitalityStore.AddResolvedVitality(tagName, std::move(vitality));

            ++built;
        }

        m_VitalityStore.Freeze();

        m_LogsService.Message("[VitalityBuilder] INFO: Built."
            " Layouts: {} | Skipped (no damage sections): {}"
            " | Kill sections: {} | Headshot sections: {} (anchored: {})",
            built, skipped, killSections, headshotSections, headshotAnchored);
    }

    auto VitalityBuilder::BuildLayout(const ResolvedHlmt& hlmt, const ResolvedModelLink* link) const -> ResolvedVitality
    {
        ResolvedVitality vitality;
        vitality.MaximumVitality = hlmt.MaximumVitality;
        vitality.MaximumShieldVitality = hlmt.MaximumShieldVitality;

        if (link)
        {
            vitality.ObjectCenter = link->ObjectCenter;
        }

        const std::size_t count = hlmt.DamageSections.size();
        vitality.Sections.resize(count);

        for (std::size_t i = 0; i < count; ++i)
        {
            const DamageSection& ds = hlmt.DamageSections[i];
            Section& out = vitality.Sections[i];

            out.NameId = ds.Name;
            out.SectionIndex = static_cast<int>(i);
            out.VitalityPercentage = ds.VitalityPercentage;
            out.StunTime = ds.StunTime;
            out.RechargeTime = ds.RechargeTime;

            if (link && i < link->SectionToRegion.size())
            {
                out.CollRegion = link->SectionToRegion[i];
            }

            if (ds.ShieldGlobalMaterialName != 0)
            {
                out.Kind = Kind::Shield;
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
                }
                if (ir.Flags & k_FlagDestroysObject)
                {
                    out.DestroysObject = true;
                }
            }

            out.Transfers.reserve(ds.SectionDamageTransfers.size());
            for (const auto& transfer : ds.SectionDamageTransfers)
            {
                Transfer t;
                t.TargetSection = transfer.DamageSectionIndex;
                t.Amount = transfer.TransferAmount;
                t.Flags = transfer.Flags;
                out.Transfers.push_back(t);
            }
        }

        // Fallback: the hlmt can name the shielded state section without a shield material
        {
            const int idx = hlmt.ShieldedStateDamageSectionIndex;
            if (idx >= 0 &&
                static_cast<std::size_t>(idx) < vitality.Sections.size())
            {
                vitality.Sections[idx].Kind = Kind::Shield;
            }
        }

        // Role lists (after all roles are known), then anchors
        for (std::size_t i = 0; i < count; ++i)
        {
            const Section& s = vitality.Sections[i];
            const int index = static_cast<int>(i);

            if (s.IsCritical) vitality.KillSections.push_back(index);
            if (s.IsHeadshot) vitality.HeadshotSections.push_back(index);
            if (s.Kind == Kind::Shield) vitality.ShieldSections.push_back(index);
        }

        if (!vitality.KillSections.empty()) vitality.CriticalSection = vitality.KillSections.front();
        if (!vitality.ShieldSections.empty()) vitality.ShieldSection = vitality.ShieldSections.front();

        if (link)
        {
            for (auto& section : vitality.Sections)
            {
                section.Aim = this->ResolveAnchor(section, *link);
            }
        }

        return vitality;
    }

    auto VitalityBuilder::ResolveAnchor(const Section& section,
        const ResolvedModelLink& link) const -> AimAnchor
    {
        // 1. ModelTarget that the hlmt assigns to this section (most relevant one)
        const AimAnchor* best = nullptr;
        for (const AimAnchor& target : link.Targets)
        {
            if (target.Source != AnchorSource::ModelTarget) continue;
            if (target.SectionIndex != section.SectionIndex) continue;

            // A Headshot lock-on marker only anchors Headshot sections (step 3)
            if (target.Headshot && !section.IsHeadshot) continue;

            if (!best || target.Relevance > best->Relevance) best = &target;
        }
        if (best) return *best;

        // 2. Bounds of the coll region that owns the section
        if (section.CollRegion >= 0 &&
            static_cast<std::size_t>(section.CollRegion) < link.RegionAnchors.size())
        {
            const AimAnchor& region = link.RegionAnchors[section.CollRegion];
            if (region.Source != AnchorSource::None) return region;
        }

        // 3. Headshot sections: the ModelTarget with the Headshot lock-on flag
        // (The hlmt usually assigns that marker to the body section, not to the head one)
        if (section.IsHeadshot)
        {
            best = nullptr;
            for (const AimAnchor& target : link.Targets)
            {
                if (target.Source != AnchorSource::ModelTarget || !target.Headshot) continue;

                if (!best || target.Relevance > best->Relevance) best = &target;
            }
            if (best)
            {
                AimAnchor anchor = *best;
                anchor.Source = AnchorSource::HeadshotTarget;
                return anchor;
            }
        }

        // 4. Center of the model (imprecise); not for shield sections
        if (section.Kind != Kind::Shield) return link.ObjectCenter;

        return AimAnchor{};
    }

    auto VitalityBuilder::Cleanup() -> void
    {
        m_VitalityStore.Cleanup();

        m_LogsService.Message("[VitalityBuilder] INFO: Cleanup completed.");
    }
}