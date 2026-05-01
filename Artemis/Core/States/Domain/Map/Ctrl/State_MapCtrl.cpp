#include "pch.h"
#include "Core/States/Domain/Map/Ctrl/State_MapCtrl.h"
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Interface/System_Debug.h"

bool State_MapCtrl::HasCtrl(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Ctrls.count(tagName) > 0;
}

const CtrlObject* State_MapCtrl::GetCtrl(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Ctrls.find(tagName);
    return it != m_Ctrls.end() ? &it->second : nullptr;
}

void State_MapCtrl::AddCtrl(const std::string& tagName, CtrlObject data)
{
    //this->LogRawData(tagName, data);

    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Ctrls.emplace(tagName, std::move(data));
}

void State_MapCtrl::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Ctrls.clear();
}

void State_MapCtrl::LogRawData(const std::string& tagName, const CtrlObject& ctrl)
{
    const auto& d = ctrl.Data;

    g_pSystem->Debug->Log("=== CTRL RAW: %s ===", tagName.c_str());

    // --- Base ---
    g_pSystem->Debug->Log("  [Base]");
    g_pSystem->Debug->Log("    ObjectType:                      %u", (uint32_t)d.ObjectType);
    g_pSystem->Debug->Log("    BoundingRadius:                  %.3f", d.BoundingRadius);
    g_pSystem->Debug->Log("    HorizontalAccelerationScale:     %.3f", d.HorizontalAccelerationScale);
    g_pSystem->Debug->Log("    VerticalAccelerationScale:       %.3f", d.VerticalAccelerationScale);
    g_pSystem->Debug->Log("    AngularAccelerationScale:        %.3f", d.AngularAccelerationScale);
    g_pSystem->Debug->Log("    Flags:                           0x%08X", d.Flags);
    g_pSystem->Debug->Log("      NotAPathfindingObstacle:       %d", (d.Flags >> 3) & 1);
    g_pSystem->Debug->Log("      EarlyMover:                    %d", (d.Flags >> 6) & 1);
    g_pSystem->Debug->Log("      DoesNotCauseCollisionDamage:   %d", (d.Flags >> 5) & 1);

    // --- Control-specific ---
    g_pSystem->Debug->Log("  [Control]");
    g_pSystem->Debug->Log("    Type:                            %u", (uint32_t)d.Type);
    g_pSystem->Debug->Log("    TriggersWhen:                    %u", (uint32_t)d.TriggersWhen);
    g_pSystem->Debug->Log("    CallValue:                       %.3f", d.CallValue);
    g_pSystem->Debug->Log("    AutomaticActivationRadius:       %.3f", d.AutomaticActivationRadius);
    g_pSystem->Debug->Log("    DelayTime:                       %.3f", d.DelayTime);
    g_pSystem->Debug->Log("    Flags_2:                         0x%08X", d.Flags_2);
    g_pSystem->Debug->Log("      PositionLoops:                 %d", (d.Flags_2 >> 0) & 1);
    g_pSystem->Debug->Log("      PositionInterpolation:         %d", (d.Flags_2 >> 2) & 1);

    // --- Transition times ---
    g_pSystem->Debug->Log("  [Transitions]");
    g_pSystem->Debug->Log("    PowerTransitionTime:             %.3f", d.PowerTransitionTime);
    g_pSystem->Debug->Log("    PowerAccelerationTime:           %.3f", d.PowerAccelerationTime);
    g_pSystem->Debug->Log("    PositionTransitionTime:          %.3f", d.PositionTransitionTime);
    g_pSystem->Debug->Log("    PositionAccelerationTime:        %.3f", d.PositionAccelerationTime);
    g_pSystem->Debug->Log("    DepoweredPositionTransitionTime: %.3f", d.DepoweredPositionTransitionTime);
    g_pSystem->Debug->Log("    DepoweredPositionAccelerationTime: %.3f", d.DepoweredPositionAccelerationTime);

    // --- AI Properties ---
    g_pSystem->Debug->Log("  [AiProperties: %zu]", ctrl.AiProperties.size());
    for (size_t i = 0; i < ctrl.AiProperties.size(); ++i)
    {
        const auto& ai = ctrl.AiProperties[i];
        g_pSystem->Debug->Log("    [%zu] Flags:0x%08X AiSize:%u LeapJumpSpeed:%u",
            i,
            ai.Flags,
            (uint32_t)ai.AiSize,
            (uint32_t)ai.LeapJumpSpeed);
        g_pSystem->Debug->Log("      DestroyableCover:        %d", (ai.Flags >> 0) & 1);
        g_pSystem->Debug->Log("      PathfindingIgnoreWhenDead: %d", (ai.Flags >> 1) & 1);
        g_pSystem->Debug->Log("      DynamicCover:            %d", (ai.Flags >> 2) & 1);
        g_pSystem->Debug->Log("      NonFlightBlocking:       %d", (ai.Flags >> 3) & 1);
        g_pSystem->Debug->Log("      DynamicCoverFromCentre:  %d", (ai.Flags >> 4) & 1);
        g_pSystem->Debug->Log("      HasCornerMarkers:        %d", (ai.Flags >> 5) & 1);
        g_pSystem->Debug->Log("      ConsiderForInteraction:  %d", (ai.Flags >> 7) & 1);
    }

    // --- MultiplayerObject ---
    g_pSystem->Debug->Log("  [MultiplayerObject: %zu]", ctrl.MultiplayerObject.size());
    for (size_t i = 0; i < ctrl.MultiplayerObject.size(); ++i)
    {
        const auto& mp = ctrl.MultiplayerObject[i];
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

    g_pSystem->Debug->Log("=== END CTRL: %s ===", tagName.c_str());
}