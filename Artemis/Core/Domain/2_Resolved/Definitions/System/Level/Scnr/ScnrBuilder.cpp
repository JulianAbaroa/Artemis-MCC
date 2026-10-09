module Resolved.Definitions.System;
import :Scnr;

import Common.Math.Type;

namespace
{
    using Common::Math::Type::Vec3;
    using Resolved::Definitions::Type::Scnr::TriggerVolumeKind;
    using Resolved::Definitions::Type::Scnr::TriggerVolume;
    using Resolved::Definitions::Type::Scnr::BoundaryTrigger;
    using Resolved::Definitions::Type::Scnr::SoftCeilingKind;
    using Resolved::Definitions::Type::Scnr::SoftCeiling;

    constexpr std::uint16_t k_LastTriggerVolumeKind{ 0x0001 };
    constexpr std::uint16_t k_LastSoftCeilingKind{ 0x0002 };

    template <typename TVec3>
    constexpr auto ToVec3(const TVec3& value) -> Vec3
    {
        return { value.X, value.Y, value.Z };
    }

    constexpr auto HasBit(std::uint32_t flags, std::uint32_t bit) -> bool
    {
        return (flags & (1u << bit)) != 0;
    }
}

namespace Resolved::Definitions::System
{
    auto ScnrBuilder::Build(const ScnrObject& scnr) -> Scnr
    {
        Scnr out{};

        out.TagName = scnr.TagName;

        BuildTriggerVolumes(scnr, out);
        BuildBoundaryTriggers(scnr, out);
        BuildSoftCeilings(scnr, out);

        return out;
    }

    auto ScnrBuilder::BuildTriggerVolumes(const ScnrObject& scnr, Scnr& out) -> void
    {
        out.TriggerVolumes.reserve(scnr.TriggerVolumes.size());

        for (const auto& volume : scnr.TriggerVolumes)
        {
            TriggerVolume built{};

            built.Name = volume.Name;
            built.Kind = volume.Type <= k_LastTriggerVolumeKind
                ? static_cast<TriggerVolumeKind>(volume.Type)
                : TriggerVolumeKind::Invalid;

            built.Position = ToVec3(volume.Position);
            built.Forward = ToVec3(volume.Forward);
            built.Up = ToVec3(volume.Up);
            built.Extents = ToVec3(volume.Extents);

            built.ZSink = volume.ZSink;
            built.SectorBoundsMin = { volume.SectorBoundsX.Min, volume.SectorBoundsY.Min, volume.SectorBoundsZ.Min };
            built.SectorBoundsMax = { volume.SectorBoundsX.Max, volume.SectorBoundsY.Max, volume.SectorBoundsZ.Max };

            built.SectorPoints.reserve(volume.SectorPoints.size());
            for (const auto& point : volume.SectorPoints)
            {
                built.SectorPoints.push_back(ToVec3(point.Position));
            }

            out.TriggerVolumes.push_back(std::move(built));
        }
    }

    auto ScnrBuilder::BuildBoundaryTriggers(const ScnrObject& scnr, Scnr& out) -> void
    {
        const auto build = [](const auto& entries, std::vector<BoundaryTrigger>& triggers) {
            triggers.reserve(entries.size());

            for (const auto& entry : entries)
            {
                BoundaryTrigger built{};

                built.TriggerVolumeIndex = entry.TriggerVolumeIndex;
                built.DontKillImmediately = HasBit(entry.Flags, 0);
                built.OnlyKillPlayers = HasBit(entry.Flags, 1);

                triggers.push_back(built);
            }
        };

        build(scnr.ScenarioKillTriggers, out.KillTriggers);
        build(scnr.ScenarioSafeZoneTriggers, out.SafeZoneTriggers);
    }

    auto ScnrBuilder::BuildSoftCeilings(const ScnrObject& scnr, Scnr& out) -> void
    {
        out.SoftCeilings.reserve(scnr.SoftCeilings.size());

        for (const auto& ceiling : scnr.SoftCeilings)
        {
            SoftCeiling built{};

            built.Name = ceiling.Name;
            built.Kind = ceiling.Type <= k_LastSoftCeilingKind
                ? static_cast<SoftCeilingKind>(ceiling.Type)
                : SoftCeilingKind::Invalid;

            built.IgnoreBipeds = HasBit(ceiling.Flags, 0);
            built.IgnoreVehicles = HasBit(ceiling.Flags, 1);
            built.IgnoreCamera = HasBit(ceiling.Flags, 2);
            built.IgnoreHugeVehicles = HasBit(ceiling.Flags, 3);

            out.SoftCeilings.push_back(built);
        }
    }
}