module;

#include "External/imgui/imgui.h"

module Gui.Color.System;
import :Group;

namespace Gui::Color::System
{
    auto GroupToColor::FromGroup(Group group) -> ImVec4
    {
        switch (group)
        {
        case Group::Actor: return { 1.0f, 0.65f, 0.65f, 1.0f };
        case Group::Mobility: return { 1.0f, 0.85f, 0.45f, 1.0f };
        case Group::Item: return { 0.6f, 1.0f, 0.6f, 1.0f };
        case Group::Hazard: return { 1.0f, 0.6f, 0.35f, 1.0f };
        // note: Lavender, the color fixtures had (D6).
        case Group::MapLogic: return { 0.75f, 0.65f, 1.0f, 1.0f };
        case Group::Scenery: return { 0.8f, 0.8f, 0.7f, 1.0f };
        default: return { 0.7f, 0.7f, 0.7f, 1.0f };
        }
    }
}