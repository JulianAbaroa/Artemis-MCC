export module Resolved.Definitions.Reflect:Hlmt;

import Common.Reflect.Type;
import Resolved.Definitions.Type;
import std;

namespace
{
    using InstantResponse = Resolved::Definitions::Type::Hlmt::InstantResponse;
    using DamageTransfer = Resolved::Definitions::Type::Hlmt::DamageTransfer;
    using DamageSection = Resolved::Definitions::Type::Hlmt::DamageSection;
    using ModelTarget = Resolved::Definitions::Type::Hlmt::ModelTarget;
    using CollisionRegion = Resolved::Definitions::Type::Hlmt::CollisionRegion;
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
    struct Fields<DamageTransfer>
    {
        static constexpr bool HasFields = true;
        static constexpr auto Value = std::tuple{
            MakeField("Flags", &DamageTransfer::Flags),
            MakeField("TransferAmount", &DamageTransfer::TransferAmount),
            MakeField("DamageSectionIndex", &DamageTransfer::DamageSectionIndex),
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
            MakeField("StunTime", &DamageSection::StunTime),
            MakeField("RechargeTime", &DamageSection::RechargeTime),
            MakeField("InstantResponses", &DamageSection::InstantResponses),
            MakeField("SectionDamageTransfers", &DamageSection::SectionDamageTransfers),
        };
    };

    template <>
    struct Fields<ModelTarget>
    {
        static constexpr bool HasFields = true;
        static constexpr auto Value = std::tuple{
            MakeField("MarkerName", &ModelTarget::MarkerName),
            MakeField("Size", &ModelTarget::Size),
            MakeField("ConeAngle", &ModelTarget::ConeAngle),
            MakeField("DamageSectionIndex", &ModelTarget::DamageSectionIndex),
            MakeField("VariantIndex", &ModelTarget::VariantIndex),
            MakeField("TargetingRelevance", &ModelTarget::TargetingRelevance),
            MakeField("LockOnFlags", &ModelTarget::LockOnFlags),
        };
    };

    template <>
    struct Fields<CollisionRegion>
    {
        static constexpr bool HasFields = true;
        static constexpr auto Value = std::tuple{
            MakeField("Name", &CollisionRegion::Name),
            MakeField("CollisionRegionIndex", &CollisionRegion::CollisionRegionIndex),
            MakeField("PhysicsRegionIndex", &CollisionRegion::PhysicsRegionIndex),
        };
    };

    template <>
    struct Fields<Hlmt>
    {
        static constexpr bool HasFields = true;
        static constexpr auto Value = std::tuple{
            MakeField("TagName", &Hlmt::TagName),
            MakeField("RenderModelTagName", &Hlmt::RenderModelTagName),
            MakeField("CollisionModelTagName", &Hlmt::CollisionModelTagName),
            MakeField("MaximumVitality", &Hlmt::MaximumVitality),
            MakeField("MaximumShieldVitality", &Hlmt::MaximumShieldVitality),
            MakeField("ShieldedStateDamageSectionIndex", &Hlmt::ShieldedStateDamageSectionIndex),
            MakeField("DamageSections", &Hlmt::DamageSections),
            MakeField("ModelTargets", &Hlmt::ModelTargets),
            MakeField("CollisionRegions", &Hlmt::CollisionRegions),
        };
    };
}