export module Blam.MapModel.Definitions.Reflect:Scnr;

import Common.Reflect.Type;
import Blam.MapModel.Definitions.Type;
import std;

namespace
{
    using Blam::MapModel::Definitions::Type::Scnr::TriggerVolume;
    using Blam::MapModel::Definitions::Type::Scnr::BoundaryTrigger;
    using Blam::MapModel::Definitions::Type::Scnr::SoftCeiling;
    using Blam::MapModel::Definitions::Type::Scnr::Scnr;
}

export namespace Common::Reflect::Type
{
    template <>
    struct Fields<TriggerVolume>
    {
        static constexpr auto k_Value = std::tuple{
            MakeField("Name", &TriggerVolume::Name),
            MakeField("Kind", &TriggerVolume::Kind),
            MakeField("Position", &TriggerVolume::Position),
            MakeField("Forward", &TriggerVolume::Forward),
            MakeField("Up", &TriggerVolume::Up),
            MakeField("Extents", &TriggerVolume::Extents),
            MakeField("ZSink", &TriggerVolume::ZSink),
            MakeField("SectorPoints", &TriggerVolume::SectorPoints),
            MakeField("SectorBoundsMin", &TriggerVolume::SectorBoundsMin),
            MakeField("SectorBoundsMax", &TriggerVolume::SectorBoundsMax),
        };
    };

    template <>
    struct Fields<BoundaryTrigger>
    {
        static constexpr auto k_Value = std::tuple{
            MakeField("TriggerVolumeIndex", &BoundaryTrigger::TriggerVolumeIndex),
            MakeField("DontKillImmediately", &BoundaryTrigger::DontKillImmediately),
            MakeField("OnlyKillPlayers", &BoundaryTrigger::OnlyKillPlayers),
        };
    };

    template <>
    struct Fields<SoftCeiling>
    {
        static constexpr auto k_Value = std::tuple{
            MakeField("Name", &SoftCeiling::Name),
            MakeField("Kind", &SoftCeiling::Kind),
            MakeField("IgnoreBipeds", &SoftCeiling::IgnoreBipeds),
            MakeField("IgnoreVehicles", &SoftCeiling::IgnoreVehicles),
            MakeField("IgnoreCamera", &SoftCeiling::IgnoreCamera),
            MakeField("IgnoreHugeVehicles", &SoftCeiling::IgnoreHugeVehicles),
        };
    };

    template <>
    struct Fields<Scnr>
    {
        static constexpr auto k_Value = std::tuple{
            MakeField("TagName", &Scnr::TagName),
            MakeField("MapId", &Scnr::MapId),
            MakeField("TriggerVolumes", &Scnr::TriggerVolumes),
            MakeField("KillTriggers", &Scnr::KillTriggers),
            MakeField("SafeZoneTriggers", &Scnr::SafeZoneTriggers),
            MakeField("SoftCeilings", &Scnr::SoftCeilings),
        };
    };
}