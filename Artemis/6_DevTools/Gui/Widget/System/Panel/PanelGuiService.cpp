module;

#include "External/imgui/imgui.h"

module DevTools.Gui.Widget.System;
import :Panel;

import DevTools.Gui.Format.System;
import DevTools.Gui.Style.Type;
import std;

namespace
{
    using DevTools::Gui::Format::System::HexFormater;

    using DevTools::Gui::Style::Type::k_SectionColor;

    const ImVec4 k_FalseColor{ 0.4f, 0.4f, 0.4f, 1.0f };
}

namespace DevTools::Gui::Widget::System
{
    auto PanelGuiService::DrawVec3(const char* label, const Vec3& value) -> void
    {
        ImGui::Text("%s %.3f  %.3f  %.3f", label, value.X, value.Y, value.Z);
    }

    auto PanelGuiService::DrawBoolBadge(const char* label, bool value, const ImVec4& trueColor) -> void
    {
        ImGui::Text("%-10s", label);
        ImGui::SameLine();

        if (value) ImGui::TextColored(trueColor, "yes");
        else       ImGui::TextColored(k_FalseColor, "no");
    }

    auto PanelGuiService::DrawSectionSeparator(const char* title) -> void
    {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (title) ImGui::TextColored(k_SectionColor, "%s", title);
    }

    auto PanelGuiService::DrawHeader(const ImVec4& color, const char* title,
        const std::string& tagName, std::uint32_t handle) -> void
    {
        ImGui::TextColored(color, "%s", title);
        ImGui::Separator();
        ImGui::Text("%s", tagName.c_str());
        ImGui::Text("Handle: %s", HexFormater::Hex32(handle).c_str());
    }
}