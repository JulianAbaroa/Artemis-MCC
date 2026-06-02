#include "pch.h"

#include "TeamToColor.h"

#include "Core/Types/Team.h"

#include "External/imgui/imgui.h"
#include "External/imgui/imgui_internal.h"

ImU32 TeamToColor::TeamColorU32(Team team, uint8_t alpha)
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

ImVec4 TeamToColor::TeamColorVec4(Team team, float alpha)
{
    switch (team)
    {
    case Team::Red:     return ImVec4(0.86f, 0.19f, 0.19f, alpha);
    case Team::Blue:    return ImVec4(0.19f, 0.39f, 0.86f, alpha);
    case Team::Green:   return ImVec4(0.19f, 0.78f, 0.19f, alpha);
    case Team::Orange:  return ImVec4(1.0f, 0.54f, 0.0f, alpha);
    case Team::Purple:  return ImVec4(0.58f, 0.19f, 0.86f, alpha);
    case Team::Gold:    return ImVec4(1.0f, 0.84f, 0.0f, alpha);
    case Team::Brown:   return ImVec4(0.54f, 0.35f, 0.16f, alpha);
    case Team::Pink:    return ImVec4(1.0f, 0.41f, 0.70f, alpha);
    default:            return ImVec4(0.70f, 0.70f, 0.70f, alpha);
    }
}