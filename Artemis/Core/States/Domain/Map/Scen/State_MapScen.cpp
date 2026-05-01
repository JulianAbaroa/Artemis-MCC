#include "pch.h"
#include "Core/States/Domain/Map/Scen/State_MapScen.h"
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Interface/System_Debug.h"

bool State_MapScen::HasScen(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Scens.count(tagName) > 0;
}

const ScenObject* State_MapScen::GetScen(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Scens.find(tagName);
    return it != m_Scens.end() ? &it->second : nullptr;
}

void State_MapScen::AddScen(const std::string& tagName, ScenObject data)
{
    //this->LogRawData(tagName, data);

    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Scens.emplace(tagName, std::move(data));
}

void State_MapScen::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Scens.clear();
}

void State_MapScen::LogRawData(const std::string& tagName, const ScenObject& scen)
{
    const auto& d = scen.Data;

    g_pSystem->Debug->Log("=== SCEN RAW: %s ===", tagName.c_str());

    // --- Base ---
    g_pSystem->Debug->Log("  [Base]");
    g_pSystem->Debug->Log("    ObjectType:                    %u", (uint32_t)d.ObjectType);
    g_pSystem->Debug->Log("    BoundingRadius:                %.3f", d.BoundingRadius);
    g_pSystem->Debug->Log("    Flags:                         0x%08X", d.Flags);
    g_pSystem->Debug->Log("      NotAPathfindingObstacle:     %d", (d.Flags >> 3) & 1);
    g_pSystem->Debug->Log("      DoesNotCauseCollisionDamage: %d", (d.Flags >> 5) & 1);
    g_pSystem->Debug->Log("      EarlyMover:                  %d", (d.Flags >> 6) & 1);

    // --- Scenery-specific ---
    g_pSystem->Debug->Log("  [Scenery]");
    g_pSystem->Debug->Log("    PathfindingPolicy:             %u", (uint32_t)d.PathfindingPolicy);
    g_pSystem->Debug->Log("    PhysicallySimulates:           %d", (d.Flags_2 >> 0) & 1);

    // --- AI Properties ---
    g_pSystem->Debug->Log("  [AiProperties: %zu]", scen.AiProperties.size());
    for (size_t i = 0; i < scen.AiProperties.size(); ++i)
    {
        const auto& ai = scen.AiProperties[i];
        g_pSystem->Debug->Log("    [%zu] Flags:0x%08X AiSize:%u LeapJumpSpeed:%u",
            i,
            ai.Flags,
            (uint32_t)ai.AiSize,
            (uint32_t)ai.LeapJumpSpeed);
        g_pSystem->Debug->Log("      DestroyableCover:          %d", (ai.Flags >> 0) & 1);
        g_pSystem->Debug->Log("      PathfindingIgnoreWhenDead: %d", (ai.Flags >> 1) & 1);
        g_pSystem->Debug->Log("      DynamicCover:              %d", (ai.Flags >> 2) & 1);
        g_pSystem->Debug->Log("      NonFlightBlocking:         %d", (ai.Flags >> 3) & 1);
        g_pSystem->Debug->Log("      DynamicCoverFromCentre:    %d", (ai.Flags >> 4) & 1);
        g_pSystem->Debug->Log("      HasCornerMarkers:          %d", (ai.Flags >> 5) & 1);
        g_pSystem->Debug->Log("      ConsiderForInteraction:    %d", (ai.Flags >> 7) & 1);
    }

    // --- MultiplayerObject ---
    g_pSystem->Debug->Log("  [MultiplayerObject: %zu]", scen.MultiplayerObject.size());
    for (size_t i = 0; i < scen.MultiplayerObject.size(); ++i)
    {
        const auto& mp = scen.MultiplayerObject[i];
        g_pSystem->Debug->Log("    [%zu] Type:%u BoundaryShape:%u"
            " BoundaryWidthRadius:%.3f BoundaryBoxLength:%.3f BoundaryHeight:%.3f"
            " DefaultSpawnTime:%d DefaultAbandonmentTime:%d"
            " GameEngineFlags:0x%02X Flags:0x%04X TeleporterPassability:0x%02X",
            i,
            (uint32_t)mp.Type,
            (uint32_t)mp.BoundaryShape,
            mp.BoundaryWidthRadius,
            mp.BoundaryBoxLength,
            mp.BoundaryHeight,
            (int32_t)mp.DefaultSpawnTime,
            (int32_t)mp.DefaultAbandonmentTime,
            (uint32_t)mp.GameEngineFlags,
            (uint32_t)mp.Flags,
            (uint32_t)mp.TeleporterPassability);
    }

    g_pSystem->Debug->Log("=== END SCEN: %s ===", tagName.c_str());
}