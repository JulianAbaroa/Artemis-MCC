module Blam.MapModel.Definitions.System;
import :Hlmt;

import Blam.MapModel.Definitions.Type;

namespace
{
    using Blam::MapModel::Definitions::Type::Hlmt::DamageTransfer;
    using Blam::MapModel::Definitions::Type::Hlmt::ModelTarget;
    using Blam::MapModel::Definitions::Type::Hlmt::RegionTransition;
    using Blam::MapModel::Definitions::Type::Hlmt::Variant;
    using Blam::MapModel::Definitions::Type::Hlmt::VariantPermutation;
    using Blam::MapModel::Definitions::Type::Hlmt::VariantRegion;
    using Blam::MapModel::Definitions::Type::Hlmt::VariantState;

    // Copies the fields of a model target entry.
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

    // Copies the fields of every damage transfer entry.
    template <typename TEntry>
    auto MakeDamageTransfers(const std::vector<TEntry>& src) -> std::vector<DamageTransfer>
    {
        std::vector<DamageTransfer> out{};
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

    // Copies the region transitions of an instant response.
    template <typename TEntry>
    auto MakeRegionTransitions(const std::vector<TEntry>& src) -> std::vector<RegionTransition>
    {
        std::vector<RegionTransition> out{};
        out.reserve(src.size());

        for (const auto& entry : src)
        {
            out.push_back({ .Region = entry.Region, .NewState = entry.NewState });
        }

        return out;
    }

    // Copies the regions of every variant, down to the permutation each damage state shows.
    template <typename TEntry>
    auto MakeVariants(const std::vector<TEntry>& src) -> std::vector<Variant>
    {
        std::vector<Variant> out{};
        out.reserve(src.size());

        for (const auto& srcVariant : src)
        {
            Variant& variant = out.emplace_back();

            for (const auto& srcRegion : srcVariant.Regions)
            {
                VariantRegion& region = variant.Regions.emplace_back();

                region.RegionName = srcRegion.RegionName;

                for (const auto& srcPermutation : srcRegion.Permutations)
                {
                    VariantPermutation& permutation = region.Permutations.emplace_back();

                    for (const auto& srcState : srcPermutation.States)
                    {
                        permutation.States.push_back({
                            .PermutationName = srcState.PermutationName,
                            .State = srcState.State });
                    }
                }
            }
        }

        return out;
    }
}

namespace Blam::MapModel::Definitions::System
{
    auto HlmtBuilder::Build(const HlmtObject& hlmt) -> Hlmt
    {
        Hlmt out{};

        out.TagName = hlmt.TagName;

        out.RenderModelTagName = m_TagReferenceService.ResolveTagReferenceName(hlmt.Data.RenderModel);
        out.CollisionModelTagName = m_TagReferenceService.ResolveTagReferenceName(hlmt.Data.CollisionModel);
        out.PhysicsModelTagName = m_TagReferenceService.ResolveTagReferenceName(hlmt.Data.PhysicsModel);

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
                    .Flags = ir.Flags,
                    .DamageThreshold = ir.DamageThreshold,
                    .RegionTransitions = MakeRegionTransitions(ir.RegionTransitions) });
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

        out.Variants = MakeVariants(hlmt.Variants);

        return out;
    }
}