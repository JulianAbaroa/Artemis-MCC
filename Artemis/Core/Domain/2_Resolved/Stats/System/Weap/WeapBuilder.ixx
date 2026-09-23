export module Resolved.Stats.System:Weap;

import Map.Tag.Type;
import Resolved.Stats.Type;

export namespace Resolved::Stats::System
{
    class WeapBuilder
    {
    private:
        using WeapObject = Map::Tag::Type::Weap::Object::WeapObject;
        using ResolvedWeap = Resolved::Stats::Type::Weap::Weap;
        using BarrelData = Resolved::Stats::Type::Weap::BarrelData;
        using MagazineData = Resolved::Stats::Type::Weap::MagazineData;
        using TriggerData = Resolved::Stats::Type::Weap::TriggerData;
        using TargetTrackingData = Resolved::Stats::Type::Weap::TargetTrackingData;

    public:
        WeapBuilder() = default;
        ~WeapBuilder() = default;

        auto Build(const WeapObject& weap) -> ResolvedWeap;

    private:
        auto BuildBarrel(const WeapObject& weap) -> BarrelData;
        auto BuildMagazine(const WeapObject& weap) -> MagazineData;
        auto BuildTrigger(const WeapObject& weap) -> TriggerData;
        auto BuildTargetTracking(const WeapObject& weap) -> TargetTrackingData;
    };
}