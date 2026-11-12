module;

#include "External/imgui/imgui.h"

module DevTools.Gui.Widget.System;
import :SectionBox;

import std;

namespace
{
    // Space between the border of the box and its content.
    constexpr float k_BoxPadding{ 10.0f };
}

namespace DevTools::Gui::Widget::System
{
    SectionBox::SectionBox(bool useFullWidth) :
        m_IsFullWidth(useFullWidth),
        m_Min(ImGui::GetCursorScreenPos()),
        m_Width(ImGui::GetContentRegionAvail().x)
    {
        ImGui::BeginGroup();
        ImGui::Spacing();
        ImGui::Indent(k_BoxPadding);
        ImGui::Spacing();
    }

    SectionBox::~SectionBox()
    {
        ImGui::Spacing();
        ImGui::Unindent(k_BoxPadding);
        ImGui::Spacing();
        ImGui::EndGroup();

        const ImVec2 size{ ImGui::GetItemRectSize() };
        const float width{ m_IsFullWidth ? m_Width : size.x + (k_BoxPadding * 2.0f) };
        const ImVec2 max{ m_Min.x + width, m_Min.y + size.y };

        ImDrawList* drawList{ ImGui::GetWindowDrawList() };
        drawList->AddRectFilled(m_Min, max, ImColor(255, 255, 255, 15), 5.0f);
        drawList->AddRect(m_Min, max, ImColor(255, 255, 255, 30), 5.0f);
    }
}