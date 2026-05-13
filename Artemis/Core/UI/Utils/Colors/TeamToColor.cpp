#include "pch.h"

// Header.
#include "TeamToColor.h"

// Types.
#include "Core/Types/Domain/Team.h"

// ImGui.
#include "External/imgui/imgui.h"
#include "External/imgui/imgui_internal.h"

ImU32 TeamToColor::TeamColor(Team team, uint8_t alpha)
{
    switch (team)
    {
    case Team::Red:     return IM_COL32(220, 50, 50, alpha);
    case Team::Blue:    return IM_COL32(50, 100, 220, alpha);
    case Team::Green:   return IM_COL32(50, 200, 50, alpha);
    case Team::Orange:  return IM_COL32(255, 140, 0, alpha);
    case Team::Purple:  return IM_COL32(150, 50, 220, alpha);
    case Team::Gold:    return IM_COL32(255, 215, 0, alpha);
    case Team::Brown:   return IM_COL32(139, 90, 43, alpha);
    case Team::Pink:    return IM_COL32(255, 105, 180, alpha);
    default:            return IM_COL32(180, 180, 180, alpha);
    }
}