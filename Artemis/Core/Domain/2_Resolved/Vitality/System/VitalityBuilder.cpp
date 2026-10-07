module Resolved.Vitality.System;

import Resolved.World.Type;

namespace
{
    using Resolved::Definitions::Type::Hlmt::DamageSection;
    using Resolved::Vitality::Type::Vitality::Kind;
    using Resolved::Vitality::Type::Vitality::Transfer;
    using Resolved::World::Type::ModelLink::AnchorSource;

    using Resolved::Vitality::Type::Constant::k_FlagKillsObject;
    using Resolved::Vitality::Type::Constant::k_FlagKillsObjectNoSolo;
    using Resolved::Vitality::Type::Constant::k_FlagDestroysObject;
    using Resolved::Vitality::Type::Constant::k_FlagHeadshot;
}

namespace Resolved::Vitality::System
{
    auto VitalityBuilder::BuildForMap() -> void
    {
        std::int32_t built{};
        std::int32_t skipped{};
        std::int32_t killSections{};
        std::int32_t headshotSections{};
        std::int32_t headshotAnchored{};

        for (const auto& [tagName, hlmt] : m_DefinitionsStore.Hlmt.All())
        {
            if (hlmt.DamageSections.empty())
            {
                ++skipped;
                continue;
            }

            const ModelLink* link{ m_WorldStore.GetModelLink(tagName) };

            Vitality vitality{ this->BuildLayout(hlmt, link) };

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

            m_VitalityStore.Add(tagName, std::move(vitality));

            ++built;
        }

        m_VitalityStore.Freeze();

        m_LogsService.Message("[VitalityBuilder] INFO: Built."
            " Layouts: {} | Skipped (no damage sections): {}"
            " | Kill sections: {} | Headshot sections: {} (anchored: {}).",
            built, skipped, killSections, headshotSections, headshotAnchored);
    }

    auto VitalityBuilder::BuildLayout(const Hlmt& hlmt, const ModelLink* link) const -> Vitality
    {
        Vitality vitality{};

        vitality.MaximumVitality = hlmt.MaximumVitality;
        vitality.MaximumShieldVitality = hlmt.MaximumShieldVitality;

        if (link)
        {
            vitality.ObjectCenter = link->ObjectCenter;
        }

        const std::size_t count{ hlmt.DamageSections.size() };
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
                Transfer resolved{};

                resolved.TargetSection = transfer.DamageSectionIndex;
                resolved.Amount = transfer.TransferAmount;
                resolved.Flags = transfer.Flags;

                out.Transfers.push_back(resolved);
            }
        }

        // Fallback: the hlmt can name the shielded state section without a shield material.
        {
            const int index{ hlmt.ShieldedStateDamageSectionIndex };
            if (index >= 0 &&
                static_cast<std::size_t>(index) < vitality.Sections.size())
            {
                vitality.Sections[index].Kind = Kind::Shield;
            }
        }

        // Role lists after all roles are known, then anchors.
        for (std::size_t i = 0; i < count; ++i)
        {
            const Section& section = vitality.Sections[i];
            const int index{ static_cast<int>(i) };

            if (section.IsCritical) vitality.KillSections.push_back(index);
            if (section.IsHeadshot) vitality.HeadshotSections.push_back(index);
            if (section.Kind == Kind::Shield) vitality.ShieldSections.push_back(index);
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
        const ModelLink& link) const -> Anchor
    {
        // 1. Model target that the hlmt assigns to this section (the most relevant one).
        const Anchor* best{};
        for (const Anchor& target : link.Targets)
        {
            if (target.Source != AnchorSource::ModelTarget) continue;
            if (target.SectionIndex != section.SectionIndex) continue;

            // A headshot lock-on marker only anchors headshot sections (step 3).
            if (target.Headshot && !section.IsHeadshot) continue;

            if (!best || target.Relevance > best->Relevance) best = &target;
        }
        if (best) return *best;

        // 2. Bounds of the coll region that owns the section.
        if (section.CollRegion >= 0 &&
            static_cast<std::size_t>(section.CollRegion) < link.RegionAnchors.size())
        {
            const Anchor& region = link.RegionAnchors[section.CollRegion];
            if (region.Source != AnchorSource::None) return region;
        }

        // 3. Headshot sections: the model target with the headshot lock-on flag.
        // The hlmt usually assigns that marker to the body section, not to the head one.
        if (section.IsHeadshot)
        {
            best = nullptr;
            for (const Anchor& target : link.Targets)
            {
                if (target.Source != AnchorSource::ModelTarget || !target.Headshot) continue;

                if (!best || target.Relevance > best->Relevance) best = &target;
            }
            if (best)
            {
                Anchor anchor{ *best };
                anchor.Source = AnchorSource::HeadshotTarget;
                return anchor;
            }
        }

        // 4. Center of the model, which is imprecise. Not for shield sections.
        if (section.Kind != Kind::Shield) return link.ObjectCenter;

        return Anchor{};
    }

    auto VitalityBuilder::Cleanup() -> void
    {
        m_VitalityStore.Cleanup();

        m_LogsService.Message("[VitalityBuilder] INFO: Cleanup completed.");
    }
}