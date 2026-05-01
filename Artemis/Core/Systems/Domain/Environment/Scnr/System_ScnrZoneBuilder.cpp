#include "pch.h"
#include "Core/Systems/Domain/Environment/Scnr/System_ScnrZoneBuilder.h"
#include "Core/Types/Domain/Domains/Environment/ScnrZones.h"
#include "Generated/Scnr/ScnrObject.h"
#include <unordered_set>
#include <cmath>

// --- Build ---

ScnrMapZones System_ScnrZoneBuilder::BuildZone(const ScnrObject& scnr)
{
    ScnrMapZones out{};
    this->BuildSpawnPoints(scnr, out);
    this->BuildTriggerVolumes(scnr, out);
    this->BuildNamedLocationVolumes(scnr, out);
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
        sp.FacingYaw = this->ExtractYawFromDegree2(raw.Facing);
        sp.InsertionPointIndex = raw.InsertionPointIndex;
        out.SpawnPoints.push_back(sp);
    }
}

// --- Trigger Volumes ---

void System_ScnrZoneBuilder::BuildTriggerVolumes(const ScnrObject& scnr, ScnrMapZones& out)
{
    const int32_t tvCount = static_cast<int32_t>(scnr.TriggerVolumes.size());

    // Build AllVolumes first without kill/safe classification
    out.AllVolumes.reserve(tvCount);
    for (const auto& tv : scnr.TriggerVolumes)
        out.AllVolumes.push_back(this->BuildTriggerVolume(tv, false));

    // Kill zones — flags bit 1 = OnlyKillPlayers
    std::unordered_set<int16_t> killIndices;
    killIndices.reserve(scnr.ScenarioKillTriggers.size());
    for (const auto& kt : scnr.ScenarioKillTriggers)
    {
        if (kt.TriggerVolumeIndex >= 0 && kt.TriggerVolumeIndex < tvCount)
            killIndices.insert(kt.TriggerVolumeIndex);
    }

    // Safe zones
    std::unordered_set<int16_t> safeIndices;
    safeIndices.reserve(scnr.ScenarioSafeZoneTriggers.size());
    for (const auto& sz : scnr.ScenarioSafeZoneTriggers)
    {
        if (sz.TriggerVolumeIndex >= 0 && sz.TriggerVolumeIndex < tvCount)
            safeIndices.insert(sz.TriggerVolumeIndex);
    }

    for (int16_t i = 0; i < static_cast<int16_t>(tvCount); ++i)
    {
        if (killIndices.count(i))
        {
            // Find the original kill trigger to read its flags
            bool onlyKillPlayers = false;
            for (const auto& kt : scnr.ScenarioKillTriggers)
            {
                if (kt.TriggerVolumeIndex == i)
                {
                    onlyKillPlayers = (kt.Flags & 0x01) != 0;
                    break;
                }
            }
            out.KillZones.push_back(
                this->BuildTriggerVolume(scnr.TriggerVolumes[i], onlyKillPlayers));
        }

        if (safeIndices.count(i))
            out.SafeZones.push_back(
                this->BuildTriggerVolume(scnr.TriggerVolumes[i], false));
    }
}

void System_ScnrZoneBuilder::BuildNamedLocationVolumes(
    const ScnrObject& scnr, ScnrMapZones& out)
{
    out.NamedLocationVolumes.reserve(scnr.NamedLocationVolumes.size());

    for (const auto& raw : scnr.NamedLocationVolumes)
    {
        ScnrNamedLocationVolume nlv{};
        nlv.LocationNameId = raw.LocationName;
        nlv.Height = raw.Height;
        nlv.Sink = raw.Sink;

        nlv.Points.reserve(raw.Points.size());
        for (const auto& p : raw.Points)
        {
            ScnrNamedLocationPoint pt{};
            pt.Position[0] = p.Position.X;
            pt.Position[1] = p.Position.Y;
            pt.Position[2] = p.Position.Z;
            pt.Normal[0] = static_cast<float>(p.Normal & 0xFFFF);
            pt.Normal[1] = static_cast<float>((p.Normal >> 16) & 0xFFFF);
            nlv.Points.push_back(pt);
        }

        out.NamedLocationVolumes.push_back(std::move(nlv));
    }
}

// --- Helpers ---

float System_ScnrZoneBuilder::ExtractYawFromDegree2(uint32_t degree2)
{
    int16_t raw = static_cast<int16_t>(degree2 & 0xFFFF);
    return static_cast<float>(raw) / 32767.0f * 3.14159265f;
}

ScnrTriggerVolume System_ScnrZoneBuilder::BuildTriggerVolume(
    const Scnr_TriggerVolumesObject& tv,
    bool onlyKillPlayers)
{
    ScnrTriggerVolume out{};
    out.NameId = tv.Name;
    out.Type = static_cast<ScnrTriggerVolumeType>(tv.Type);
    out.OnlyKillPlayers = onlyKillPlayers;
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
