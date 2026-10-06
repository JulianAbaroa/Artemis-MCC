module Resolved.Definitions.System;
import :Hlmt;

import std;

namespace
{
    using DamageTransfer = Resolved::Definitions::Type::Hlmt::DamageTransfer;
    using ModelTarget = Resolved::Definitions::Type::Hlmt::ModelTarget;

    template <typename TEntry>
    auto MakeModelTarget(const TEntry& src) -> ModelTarget
    {
        ModelTarget out{};

        out.MarkerName = src.MarkerName;
        out.Size = src.Size;
        out.ConeAngle = src.ConeAngle;
        out.DamageSectionIndex = src.DamageSectionIndex;
        out.VariantIndex = src.VariantIndex;
        out.TargetingRelevance = src.TargetingRelevance;
        out.LockOnFlags = src.LockOnFlags;

        return out;
    }

    template <typename TEntry>
    auto MakeDamageTransfers(const std::vector<TEntry>& src) -> std::vector<DamageTransfer>
    {
        std::vector<DamageTransfer> out;
        out.reserve(src.size());

        for (const auto& entry : src)
        {
            DamageTransfer transfer{};

            transfer.Flags = entry.Flags;
            transfer.TransferAmount = entry.TransferAmount;
            transfer.DamageSectionIndex = entry.DamageSectionIndex;

            out.push_back(transfer);
        }

        return out;
    }
}

namespace Resolved::Definitions::System
{
    auto HlmtBuilder::Build(const HlmtObject& hlmt) -> ResolvedHlmt
    {
        ResolvedHlmt out{};

        out.TagName = hlmt.TagName;

        out.RenderModelTagName = m_TagResolverService.ResolveTagReferenceName(hlmt.Data.RenderModel);
        out.CollisionModelTagName = m_TagResolverService.ResolveTagReferenceName(hlmt.Data.CollisionModel);

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
            section.StunTime = ds.StunTime;
            section.RechargeTime = ds.RechargeTime;

            section.InstantResponses.reserve(ds.InstantResponses.size());

            for (const auto& ir : ds.InstantResponses)
            {
                section.InstantResponses.push_back(InstantResponse{
                    .Flags = ir.Flags});
            }

            section.SectionDamageTransfers = MakeDamageTransfers(ds.SectionDamageTransfers);
        }

        out.ModelTargets.reserve(hlmt.ModelTargets.size());
        for (const auto& target : hlmt.ModelTargets)
        {
            out.ModelTargets.push_back(MakeModelTarget(target));
        }

        out.CollisionRegions.reserve(hlmt.CollisionRegions.size());
        for (const auto& region : hlmt.CollisionRegions)
        {
            out.CollisionRegions.push_back({
                .Name = region.Name,
                .CollisionRegionIndex = region.CollisionRegionIndex,
                .PhysicsRegionIndex = region.PhysicsRegionIndex });
        }

        return out;
    }
}