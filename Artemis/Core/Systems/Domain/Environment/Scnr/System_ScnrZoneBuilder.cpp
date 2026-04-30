#include "pch.h"
#include "Core/Systems/Domain/Environment/Scnr/System_ScnrZoneBuilder.h"
#include <unordered_set>
#include <cmath>

// --- Build ---

ScnrMapZones System_ScnrZoneBuilder::BuildZone(const ScnrObject& scnr)
{
    ScnrMapZones out{};
    this->BuildSpawnPoints(scnr, out);
    this->BuildTriggerVolumes(scnr, out);
    return out;
}

// --- Spawn Points ---

void System_ScnrZoneBuilder::BuildSpawnPoints(const ScnrObject& scnr, ScnrMapZones& out)
{
    out.SpawnPoints.reserve(scnr.PlayerStartingLocations.size());

    for (const auto& raw : scnr.PlayerStartingLocations)
    {
        ScnrSpawnPoint sp{};
        sp.Position[0] = raw.Position.X;
        sp.Position[1] = raw.Position.Y;
        sp.Position[2] = raw.Position.Z;
        sp.FacingYaw = ExtractYawFromDegree2(raw.Facing);
        sp.InsertionPointIndex = raw.InsertionPointIndex;
        out.SpawnPoints.push_back(sp);
    }
}

// --- Trigger Volumes ---

void System_ScnrZoneBuilder::BuildTriggerVolumes(const ScnrObject& scnr, ScnrMapZones& out)
{
    // We build AllVolumes first, base reference.
    out.AllVolumes.reserve(scnr.TriggerVolumes.size());
    for (const auto& tv : scnr.TriggerVolumes)
    {
        out.AllVolumes.push_back(BuildTriggerVolume(tv));
    }

    // Kill zone indices.
    std::unordered_set<int16_t> killIndices;
    killIndices.reserve(scnr.ScenarioKillTriggers.size());
    for (const auto& kt : scnr.ScenarioKillTriggers)
    {
        if (kt.TriggerVolumeIndex >= 0 &&
            kt.TriggerVolumeIndex < static_cast<int16_t>(scnr.TriggerVolumes.size()))
        {
            killIndices.insert(kt.TriggerVolumeIndex);
        }
    }

    // Safe zone indices.
    std::unordered_set<int16_t> safeIndices;
    safeIndices.reserve(scnr.ScenarioSafeZoneTriggers.size());
    for (const auto& sz : scnr.ScenarioSafeZoneTriggers)
    {
        if (sz.TriggerVolumeIndex >= 0 &&
            sz.TriggerVolumeIndex < static_cast<int16_t>(scnr.TriggerVolumes.size()))
        {
            safeIndices.insert(sz.TriggerVolumeIndex);
        }
    }

    // We populate KillZones and SafeZones from pre-built AllVolumes.
    for (int16_t i = 0; i < static_cast<int16_t>(scnr.TriggerVolumes.size()); ++i)
    {
        if (killIndices.count(i))
        {
            out.KillZones.push_back(out.AllVolumes[i]);
        }

        if (safeIndices.count(i))
        {
            out.SafeZones.push_back(out.AllVolumes[i]);
        }
    }
}

// --- Helpers ---

// degree2 in scnr is stored as two packed floats in a uint32.
// The first float(bits 0 - 15 normalized) is the yaw(horizontal facing).
// We extract the yaw in radians directly from the raw field.
float System_ScnrZoneBuilder::ExtractYawFromDegree2(uint32_t degree2)
{
    // degree2 = two normalized int16s [-32768, 32767] → [-π, π]
    // The yaw component is the first int16 (bits [31:16] big-endian in the tag,
    // but in memory little-endian are the low bits).
    int16_t raw = static_cast<int16_t>(degree2 & 0xFFFF);
    return static_cast<float>(raw) / 32767.0f * 3.14159265f;
}

ScnrTriggerVolume System_ScnrZoneBuilder::BuildTriggerVolume(const Scnr_TriggerVolumesObject& tv)
{
    ScnrTriggerVolume out{};
    out.NameId = tv.Name;
    out.Type = static_cast<ScnrTriggerVolumeType>(tv.Type);
    out.Position[0] = tv.Position.X;
    out.Position[1] = tv.Position.Y;
    out.Position[2] = tv.Position.Z;
    out.Extents[0] = tv.Extents.X;
    out.Extents[1] = tv.Extents.Y;
    out.Extents[2] = tv.Extents.Z;
    out.Forward[0] = tv.Forward.X;
    out.Forward[1] = tv.Forward.Y;
    out.Forward[2] = tv.Forward.Z;
    out.Up[0] = tv.Up.X;
    out.Up[1] = tv.Up.Y;
    out.Up[2] = tv.Up.Z;
    return out;
}