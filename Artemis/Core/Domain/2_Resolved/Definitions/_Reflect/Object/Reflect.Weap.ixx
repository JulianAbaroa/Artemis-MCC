export module Resolved.Definitions.Reflect:Weap;

import :Object;

import Common.Reflect.Type;
import Resolved.Definitions.Type;
import std;

namespace
{
    using Resolved::Definitions::Type::Weap::Magazine;
    using Resolved::Definitions::Type::Weap::Trigger;
    using Resolved::Definitions::Type::Weap::Barrel;
    using Resolved::Definitions::Type::Weap::Weap;
}

export namespace Common::Reflect::Type
{
    template <>
    struct Fields<Magazine>
    {
        static constexpr auto k_Value = std::tuple{
            MakeField("RoundsRecharged", &Magazine::RoundsRecharged),
            MakeField("RoundsTotalInitial", &Magazine::RoundsTotalInitial),
            MakeField("RoundsTotalMaximum", &Magazine::RoundsTotalMaximum),
            MakeField("RoundsLoadedMaximum", &Magazine::RoundsLoadedMaximum),
            MakeField("RoundsReloaded", &Magazine::RoundsReloaded),
        };
    };

    template <>
    struct Fields<Trigger>
    {
        static constexpr auto k_Value = std::tuple{
            MakeField("Behavior", &Trigger::Behavior),
            MakeField("PrimaryAction", &Trigger::PrimaryAction),
            MakeField("SecondaryAction", &Trigger::SecondaryAction),
            MakeField("PrimaryBarrelIndex", &Trigger::PrimaryBarrelIndex),
            MakeField("SecondaryBarrelIndex", &Trigger::SecondaryBarrelIndex),
            MakeField("ChargingTime", &Trigger::ChargingTime),
            MakeField("ChargedTime", &Trigger::ChargedTime),
            MakeField("OverchargedAction", &Trigger::OverchargedAction),
            MakeField("ChargeFlags", &Trigger::ChargeFlags),
            MakeField("ChargedDrainRate", &Trigger::ChargedDrainRate),
        };
    };

    template <>
    struct Fields<Barrel>
    {
        static constexpr auto k_Value = std::tuple{
            MakeField("RoundsPerSecondMin", &Barrel::RoundsPerSecondMin),
            MakeField("RoundsPerSecondMax", &Barrel::RoundsPerSecondMax),
            MakeField("MagazineIndex", &Barrel::MagazineIndex),
            MakeField("RoundsPerShot", &Barrel::RoundsPerShot),
            MakeField("MinimumRoundsLoaded", &Barrel::MinimumRoundsLoaded),
            MakeField("ProjectilesPerShot", &Barrel::ProjectilesPerShot),
            MakeField("FireRecoveryTime", &Barrel::FireRecoveryTime),
            MakeField("HeatGeneratedPerRound", &Barrel::HeatGeneratedPerRound),
            MakeField("DamageReportingType", &Barrel::DamageReportingType),
        };
    };

    template <>
    struct Fields<Weap>
    {
        static constexpr auto k_Value = std::tuple{
            MakeField("Base", &Weap::Base),
            MakeField("AutoaimRange", &Weap::AutoaimRange),
            MakeField("WeaponType", &Weap::WeaponType),
            MakeField("ReadyTime", &Weap::ReadyTime),
            MakeField("HeatRecoveryThreshold", &Weap::HeatRecoveryThreshold),
            MakeField("OverheatedThreshold", &Weap::OverheatedThreshold),
            MakeField("HeatLossPerSecond", &Weap::HeatLossPerSecond),
            MakeField("OverheatedHeatLossPerSecond", &Weap::OverheatedHeatLossPerSecond),
            MakeField("HeatVentingTime", &Weap::HeatVentingTime),
            MakeField("TrackingAcquireTime", &Weap::TrackingAcquireTime),
            MakeField("HasTargetTracking", &Weap::HasTargetTracking),
            MakeField("Magazines", &Weap::Magazines),
            MakeField("Triggers", &Weap::Triggers),
            MakeField("Barrels", &Weap::Barrels),
        };
    };
}