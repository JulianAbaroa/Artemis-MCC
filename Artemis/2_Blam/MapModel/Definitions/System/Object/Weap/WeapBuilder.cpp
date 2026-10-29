module Blam.MapModel.Definitions.System;
import :Weap;

import Blam.MapModel.Definitions.Type;

namespace
{
    using Blam::MapModel::Definitions::Type::Weap::Magazine;
    using Blam::MapModel::Definitions::Type::Weap::Trigger;
    using Blam::MapModel::Definitions::Type::Weap::Barrel;
}

namespace Blam::MapModel::Definitions::System
{
    auto WeapBuilder::Build(const WeapObject& weap) -> Weap
    {
        Weap out{};

        out.Base = m_ObjectBuilder.Build(weap.TagName, weap.Data, weap.MultiplayerObject);

        out.AutoaimRange = weap.Data.AutoaimRange;
        out.WeaponType = weap.Data.WeaponType;
        out.ReadyTime = weap.Data.ReadyTime;

        out.HeatRecoveryThreshold = weap.Data.HeatRecoveryThreshold;
        out.OverheatedThreshold = weap.Data.OverheatedThreshold;
        out.HeatLossPerSecond = weap.Data.HeatLossPerSecond;
        out.OverheatedHeatLossPerSecond = weap.Data.OverheatedHeatLossPerSecond;
        out.HeatVentingTime = weap.Data.HeatVentingTime;

        out.HasTargetTracking = !weap.TargetTracking.empty();
        if (out.HasTargetTracking) out.TrackingAcquireTime = weap.TargetTracking.front().AcquireTime;

        out.Magazines.reserve(weap.Magazines.size());
        for (const auto& source : weap.Magazines)
        {
            out.Magazines.push_back(Magazine
            {
                .RoundsRecharged = source.RoundsRecharged,
                .RoundsTotalInitial = source.RoundsTotalInitial,
                .RoundsTotalMaximum = source.RoundsTotalMaximum,
                .RoundsLoadedMaximum = source.RoundsLoadedMaximum,
                .RoundsReloaded = source.RoundsReloaded,
            });
        }

        out.Triggers.reserve(weap.NewTriggers.size());
        for (const auto& source : weap.NewTriggers)
        {
            out.Triggers.push_back(Trigger
            {
                .Behavior = source.Behavior,
                .PrimaryAction = source.PrimaryAction,
                .SecondaryAction = source.SecondaryAction,
                .PrimaryBarrelIndex = source.PrimaryBarrelIndex,
                .SecondaryBarrelIndex = source.SecondaryBarrelIndex,
                .ChargingTime = source.ChargingTime,
                .ChargedTime = source.ChargedTime,
                .OverchargedAction = source.OverchargedAction,
                .ChargeFlags = source.ChargeFlags,
                .ChargedDrainRate = source.ChargedDrainRate,
            });
        }

        out.Barrels.reserve(weap.Barrels.size());
        for (const auto& source : weap.Barrels)
        {
            out.Barrels.push_back(Barrel
            {
                .RoundsPerSecondMin = source.RoundsPerSecond.Min,
                .RoundsPerSecondMax = source.RoundsPerSecond.Max,
                .MagazineIndex = source.MagazineIndex,
                .RoundsPerShot = source.RoundsPerShot,
                .MinimumRoundsLoaded = source.MinimumRoundsLoaded,
                .ProjectilesPerShot = source.ProjectilesPerShot,
                .FireRecoveryTime = source.FireRecoveryTime,
                .HeatGeneratedPerRound = source.HeatGeneratedPerRound,
                .DamageReportingType = source.DamageEffectReportingType,
            });
        }

        return out;
    }
}