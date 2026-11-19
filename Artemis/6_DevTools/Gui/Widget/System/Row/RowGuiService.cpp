module;

#include "External/imgui/imgui.h"

module DevTools.Gui.Widget.System;
import :Row;
import :Status;

import std;

namespace
{
    // Share of a row the label takes before its item, and the narrowest an item may get (in font sizes) before it goes under its label.
    constexpr float k_LabelColumnFraction{ 0.4f };
    constexpr float k_MinimumItemEms{ 6.0f };
}

namespace DevTools::Gui::Widget::System
{
    auto RowGuiService::LabelBeforeItem(const char* label, const char* tooltip) -> void
    {
        const float startX{ ImGui::GetCursorPosX() };
        const float available{ ImGui::GetContentRegionAvail().x };
        const float column{ (std::max)(available * k_LabelColumnFraction, ImGui::CalcTextSize(label).x + ImGui::GetStyle().ItemSpacing.x) };

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label);
        if (tooltip != nullptr) StatusGuiService::ShowTooltip(tooltip);

        if (available - column >= ImGui::GetFontSize() * k_MinimumItemEms) ImGui::SameLine(startX + column);
        ImGui::SetNextItemWidth(-(std::numeric_limits<float>::min)());
    }
}