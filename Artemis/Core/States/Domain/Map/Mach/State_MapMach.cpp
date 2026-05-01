#include "pch.h"
#include "Core/States/Domain/Map/Mach/State_MapMach.h"
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Interface/System_Debug.h"

bool State_MapMach::HasMach(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Machs.count(tagName) > 0;
}

const MachObject* State_MapMach::GetMach(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Machs.find(tagName);
    return it != m_Machs.end() ? &it->second : nullptr;
}

void State_MapMach::AddMach(const std::string& tagName, MachObject data)
{
    this->LogRawData(tagName, data);

    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Machs.emplace(tagName, std::move(data));
}

void State_MapMach::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Machs.clear();
}

void State_MapMach::LogRawData(const std::string& tagName, const MachObject& mach)
{
    const auto& d = mach.Data;

    g_pSystem->Debug->Log("=== MACH RAW: %s ===", tagName.c_str());

    // --- Base ---
    g_pSystem->Debug->Log("  [Base]");
    g_pSystem->Debug->Log("    ObjectType:                      %u", (uint32_t)d.ObjectType);
    g_pSystem->Debug->Log("    BoundingRadius:                  %.3f", d.BoundingRadius);
    g_pSystem->Debug->Log("    Flags:                           0x%08X", d.Flags);
    g_pSystem->Debug->Log("      NotAPathfindingObstacle:       %d", (d.Flags >> 3) & 1);
    g_pSystem->Debug->Log("      DoesNotCauseCollisionDamage:   %d", (d.Flags >> 5) & 1);
    g_pSystem->Debug->Log("      EarlyMover:                    %d", (d.Flags >> 6) & 1);

    // --- Machine-specific ---
    g_pSystem->Debug->Log("  [Machine]");
    g_pSystem->Debug->Log("    Type:                            %u", (uint32_t)d.Type);
    g_pSystem->Debug->Log("    Flags_3:                         0x%04X", d.Flags_3);
    g_pSystem->Debug->Log("      PathfindingObstacle:           %d", (d.Flags_3 >> 0) & 1);
    g_pSystem->Debug->Log("      ButNotWhenOpen:                %d", (d.Flags_3 >> 1) & 1);
    g_pSystem->Debug->Log("      Elevator:                      %d", (d.Flags_3 >> 2) & 1);
    g_pSystem->Debug->Log("    PathfindingPolicy:               %u", (uint32_t)d.PathfindingPolicy);
    g_pSystem->Debug->Log("    CollisionResponse:               %u", (uint32_t)d.CollisionResponse);
    g_pSystem->Debug->Log("    DoorOpenTime:                    %.3f", d.DoorOpenTime);
    g_pSystem->Debug->Log("    AutomaticActivationRadius:       %.3f", d.AutomaticActivationRadius);
    g_pSystem->Debug->Log("    DelayTime:                       %.3f", d.DelayTime);

    // --- Transitions ---
    g_pSystem->Debug->Log("  [Transitions]");
    g_pSystem->Debug->Log("    PowerTransitionTime:             %.3f", d.PowerTransitionTime);
    g_pSystem->Debug->Log("    PowerAccelerationTime:           %.3f", d.PowerAccelerationTime);
    g_pSystem->Debug->Log("    PositionTransitionTime:          %.3f", d.PositionTransitionTime);
    g_pSystem->Debug->Log("    PositionAccelerationTime:        %.3f", d.PositionAccelerationTime);
    g_pSystem->Debug->Log("    DepoweredPositionTransitionTime: %.3f", d.DepoweredPositionTransitionTime);
    g_pSystem->Debug->Log("    Flags_2:                         0x%08X", d.Flags_2);
    g_pSystem->Debug->Log("      PositionLoops:                 %d", (d.Flags_2 >> 0) & 1);
    g_pSystem->Debug->Log("      PositionInterpolation:         %d", (d.Flags_2 >> 2) & 1);

    // --- AI Properties ---
    g_pSystem->Debug->Log("  [AiProperties: %zu]", mach.AiProperties.size());
    for (size_t i = 0; i < mach.AiProperties.size(); ++i)
    {
        const auto& ai = mach.AiProperties[i];
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
    g_pSystem->Debug->Log("  [MultiplayerObject: %zu]", mach.MultiplayerObject.size());
    for (size_t i = 0; i < mach.MultiplayerObject.size(); ++i)
    {
        const auto& mp = mach.MultiplayerObject[i];
        g_pSystem->Debug->Log("    [%zu] Type:%u BoundaryShape:%u"
            " BoundaryWidthRadius:%.3f BoundaryBoxLength:%.3f BoundaryHeight:%.3f"
            " DefaultSpawnTime:%d DefaultAbandonmentTime:%d"
            " GameEngineFlags:0x%02X Flags:0x%04X",
            i,
            (uint32_t)mp.Type,
            (uint32_t)mp.BoundaryShape,
            mp.BoundaryWidthRadius,
            mp.BoundaryBoxLength,
            mp.BoundaryHeight,
            (int32_t)mp.DefaultSpawnTime,
            (int32_t)mp.DefaultAbandonmentTime,
            (uint32_t)mp.GameEngineFlags,
            (uint32_t)mp.Flags);
    }

    g_pSystem->Debug->Log("=== END MACH: %s ===", tagName.c_str());
}