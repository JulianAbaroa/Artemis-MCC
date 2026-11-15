module;

#include "External/imgui/imgui_internal.h"

module DevTools.Gui.Color.System;
import :Team;

namespace DevTools::Gui::Color::System
{
    auto TeamToColor::TeamColorVec4(Team team, float alpha) -> ImVec4
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
}