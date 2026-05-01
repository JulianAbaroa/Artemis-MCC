#include "pch.h"
#include "Core/States/Domain/Map/Scnr/State_MapScnr.h"
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Interface/System_Debug.h"

bool State_MapScnr::HasScnr(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Scnrs.count(tagName) > 0;
}

const ScnrObject* State_MapScnr::GetScnr(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Scnrs.find(tagName);
    return it != m_Scnrs.end() ? &it->second : nullptr;
}

void State_MapScnr::AddScnr(const std::string& tagName, ScnrObject data)
{
    //this->LogRawData(tagName, data);

    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Scnrs.emplace(tagName, std::move(data));
}

void State_MapScnr::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Scnrs.clear();
}

void State_MapScnr::LogRawData(const std::string& tagName, const ScnrObject& scnr)
{
    const auto& d = scnr.Data;

    g_pSystem->Debug->Log("=== SCNR RAW: %s ===", tagName.c_str());

    // --- Map Info ---
    g_pSystem->Debug->Log("  [MapInfo]");
    g_pSystem->Debug->Log("    MapType:                     %u", (uint32_t)d.MapType);
    g_pSystem->Debug->Log("    MapId:                       %d", d.MapId);
    g_pSystem->Debug->Log("    CampaignId:                  %d", d.CampaignId);
    g_pSystem->Debug->Log("    LocalNorth:                  %.3f", d.LocalNorth);
    g_pSystem->Debug->Log("    LocalSeaLevel:               %.3f", d.LocalSeaLevel);
    g_pSystem->Debug->Log("    AltitudeCap:                 %.3f", d.AltitudeCap);
    g_pSystem->Debug->Log("    SandboxBudget:               %.3f", d.SandboxBudget);
    g_pSystem->Debug->Log("    Flags:                       0x%04X", (uint32_t)d.Flags);
    g_pSystem->Debug->Log("      SymmetricMultiplayerMap:   %d", (d.Flags >> 2) & 1);
    g_pSystem->Debug->Log("      InSpace:                   %d", (d.Flags >> 12) & 1);
    g_pSystem->Debug->Log("      Survival:                  %d", (d.Flags >> 13) & 1);

    // --- Block counts summary ---
    g_pSystem->Debug->Log("  [BlockCounts]");
    g_pSystem->Debug->Log("    StructureBsps:               %zu", scnr.StructureBsps.size());
    g_pSystem->Debug->Log("    ObjectNames:                 %zu", scnr.ObjectNames.size());
    g_pSystem->Debug->Log("    Scenery:                     %zu", scnr.Scenery.size());
    g_pSystem->Debug->Log("    Bipeds:                      %zu", scnr.Bipeds.size());
    g_pSystem->Debug->Log("    Vehicles:                    %zu", scnr.Vehicles.size());
    g_pSystem->Debug->Log("    Equipment:                   %zu", scnr.Equipment.size());
    g_pSystem->Debug->Log("    Weapons:                     %zu", scnr.Weapons.size());
    g_pSystem->Debug->Log("    Machines:                    %zu", scnr.Machines.size());
    g_pSystem->Debug->Log("    Controls:                    %zu", scnr.Controls.size());
    g_pSystem->Debug->Log("    Crates:                      %zu", scnr.Crates.size());
    g_pSystem->Debug->Log("    TriggerVolumes:              %zu", scnr.TriggerVolumes.size());
    g_pSystem->Debug->Log("    ScenarioKillTriggers:        %zu", scnr.ScenarioKillTriggers.size());
    g_pSystem->Debug->Log("    ScenarioSafeZoneTriggers:    %zu", scnr.ScenarioSafeZoneTriggers.size());
    g_pSystem->Debug->Log("    PlayerStartingLocations:     %zu", scnr.PlayerStartingLocations.size());
    g_pSystem->Debug->Log("    PlayerStartingProfile:       %zu", scnr.PlayerStartingProfile.size());
    g_pSystem->Debug->Log("    Squads:                      %zu", scnr.Squads.size());
    g_pSystem->Debug->Log("    Zones:                       %zu", scnr.Zones.size());
    g_pSystem->Debug->Log("    NamedLocationVolumes:        %zu", scnr.NamedLocationVolumes.size());
    g_pSystem->Debug->Log("    ZoneSets:                    %zu", scnr.ZoneSets.size());
    g_pSystem->Debug->Log("    ZoneSetSwitchTriggerVolumes: %zu", scnr.ZoneSetSwitchTriggerVolumes.size());
    g_pSystem->Debug->Log("    Decals:                      %zu", scnr.Decals.size());
    g_pSystem->Debug->Log("    SoftCeilings:                %zu", scnr.SoftCeilings.size());

    // --- TriggerVolumes detail ---
    g_pSystem->Debug->Log("  [TriggerVolumes: %zu]", scnr.TriggerVolumes.size());
    for (size_t i = 0; i < scnr.TriggerVolumes.size(); ++i)
    {
        const auto& tv = scnr.TriggerVolumes[i];
        g_pSystem->Debug->Log("    [%zu] Name:%u Type:%u"
            " Pos:[%.2f,%.2f,%.2f] Extents:[%.2f,%.2f,%.2f]"
            " KillTriggerVolumeIndex:%d",
            i,
            tv.Name,
            (uint32_t)tv.Type,
            tv.Position.X, tv.Position.Y, tv.Position.Z,
            tv.Extents.X, tv.Extents.Y, tv.Extents.Z,
            (int32_t)tv.KillTriggerVolumeIndex);
    }

    // --- ScenarioKillTriggers detail ---
    g_pSystem->Debug->Log("  [ScenarioKillTriggers: %zu]", scnr.ScenarioKillTriggers.size());
    for (size_t i = 0; i < scnr.ScenarioKillTriggers.size(); ++i)
    {
        const auto& kt = scnr.ScenarioKillTriggers[i];
        g_pSystem->Debug->Log("    [%zu] TriggerVolumeIndex:%d Flags:0x%02X",
            i,
            (int32_t)kt.TriggerVolumeIndex,
            (uint32_t)kt.Flags);
    }

    // --- ScenarioSafeZoneTriggers detail ---
    g_pSystem->Debug->Log("  [ScenarioSafeZoneTriggers: %zu]", scnr.ScenarioSafeZoneTriggers.size());
    for (size_t i = 0; i < scnr.ScenarioSafeZoneTriggers.size(); ++i)
    {
        const auto& sz = scnr.ScenarioSafeZoneTriggers[i];
        g_pSystem->Debug->Log("    [%zu] TriggerVolumeIndex:%d Flags:0x%02X",
            i,
            (int32_t)sz.TriggerVolumeIndex,
            (uint32_t)sz.Flags);
    }

    // --- PlayerStartingLocations detail ---
    g_pSystem->Debug->Log("  [PlayerStartingLocations: %zu]", scnr.PlayerStartingLocations.size());
    for (size_t i = 0; i < scnr.PlayerStartingLocations.size(); ++i)
    {
        const auto& pl = scnr.PlayerStartingLocations[i];
        g_pSystem->Debug->Log("    [%zu] Pos:[%.2f,%.2f,%.2f] Facing:0x%08X"
            " InsertionPointIndex:%d Flags:0x%04X",
            i,
            pl.Position.X, pl.Position.Y, pl.Position.Z,
            pl.Facing,
            (int32_t)pl.InsertionPointIndex,
            (uint32_t)pl.Flags);
    }

    // --- NamedLocationVolumes detail ---
    g_pSystem->Debug->Log("  [NamedLocationVolumes: %zu]", scnr.NamedLocationVolumes.size());
    for (size_t i = 0; i < scnr.NamedLocationVolumes.size(); ++i)
    {
        const auto& nlv = scnr.NamedLocationVolumes[i];
        g_pSystem->Debug->Log("    [%zu] LocationName:%u Height:%.3f Sink:%.3f Points:%zu",
            i,
            nlv.LocationName,
            nlv.Height,
            nlv.Sink,
            nlv.Points.size());
    }

    // --- SoftCeilings detail ---
    g_pSystem->Debug->Log("  [SoftCeilings: %zu]", scnr.SoftCeilings.size());
    for (size_t i = 0; i < scnr.SoftCeilings.size(); ++i)
    {
        const auto& sc = scnr.SoftCeilings[i];
        g_pSystem->Debug->Log("    [%zu] Name:%u Type:%u Flags:0x%04X",
            i,
            sc.Name,
            (uint32_t)sc.Type,
            (uint32_t)sc.Flags);
    }

    // --- ZoneSetSwitchTriggerVolumes detail ---
    g_pSystem->Debug->Log("  [ZoneSetSwitchTriggerVolumes: %zu]",
        scnr.ZoneSetSwitchTriggerVolumes.size());
    for (size_t i = 0; i < scnr.ZoneSetSwitchTriggerVolumes.size(); ++i)
    {
        const auto& zs = scnr.ZoneSetSwitchTriggerVolumes[i];
        g_pSystem->Debug->Log("    [%zu] TriggerVolumeIndex:%d"
            " BeginZoneSetIndex:%d CommitZoneSetIndex:%d Flags:0x%04X",
            i,
            (int32_t)zs.TriggerVolumeIndex,
            (int32_t)zs.BeginZoneSetIndex,
            (int32_t)zs.CommitZoneSetIndex,
            (uint32_t)zs.Flags);
    }

    g_pSystem->Debug->Log("=== END SCNR: %s ===", tagName.c_str());
}