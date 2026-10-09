export module Resolved.Definitions.Reflect:Scnr;

import Common.Reflect.Type;
import Resolved.Definitions.Type;
import std;

namespace
{
    using Resolved::Definitions::Type::Scnr::TriggerVolume;
    using Resolved::Definitions::Type::Scnr::BoundaryTrigger;
    using Resolved::Definitions::Type::Scnr::SoftCeiling;
    using Resolved::Definitions::Type::Scnr::Scnr;
}

export namespace Common::Reflect::Type
{
    template <>
    struct Fields<TriggerVolume>
    {
        static constexpr bool HasFields{ true };
        static constexpr auto Value = std::tuple{
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
        static constexpr bool HasFields{ true };
        static constexpr auto Value = std::tuple{
            MakeField("TriggerVolumeIndex", &BoundaryTrigger::TriggerVolumeIndex),
            MakeField("DontKillImmediately", &BoundaryTrigger::DontKillImmediately),
            MakeField("OnlyKillPlayers", &BoundaryTrigger::OnlyKillPlayers),
        };
    };

    template <>
    struct Fields<SoftCeiling>
    {
        static constexpr bool HasFields{ true };
        static constexpr auto Value = std::tuple{
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
        static constexpr bool HasFields{ true };
        static constexpr auto Value = std::tuple{
            MakeField("TagName", &Scnr::TagName),
            MakeField("TriggerVolumes", &Scnr::TriggerVolumes),
            MakeField("KillTriggers", &Scnr::KillTriggers),
            MakeField("SafeZoneTriggers", &Scnr::SafeZoneTriggers),
            MakeField("SoftCeilings", &Scnr::SoftCeilings),
        };
    };
}