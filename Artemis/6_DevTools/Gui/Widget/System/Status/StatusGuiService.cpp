module;

#include "External/imgui/imgui.h"

module DevTools.Gui.Widget.System;
import :Status;

import std;

namespace
{
    using DevTools::Gui::Style::Type::StatusKind;

    using DevTools::Gui::Style::Type::k_StatusColors;
    using DevTools::Gui::Style::Type::k_TooltipWrapChars;

    // return: The color of the kind, with the given alpha.
    auto ColorOf(StatusKind kind, float alpha) -> ImVec4
    {
        ImVec4 color{ k_StatusColors[static_cast<std::size_t>(kind)] };
        color.w = alpha;
        return color;
    }
}

namespace DevTools::Gui::Widget::System
{
    auto StatusGuiService::DrawChip(const char* text, StatusKind kind, const char* tooltip) -> void
    {
        const ImGuiStyle& style{ ImGui::GetStyle() };
        const ImVec2 textSize{ ImGui::CalcTextSize(text) };
        const ImVec2 size{ textSize.x + (style.FramePadding.x * 2.0f), textSize.y + (style.FramePadding.y * 2.0f) };
        const ImVec2 min{ ImGui::GetCursorScreenPos() };
        const ImVec2 max{ min.x + size.x, min.y + size.y };

        ImDrawList* drawList{ ImGui::GetWindowDrawList() };
        drawList->AddRectFilled(min, max, ImGui::ColorConvertFloat4ToU32(ColorOf(kind, 0.22f)), size.y * 0.5f);
        drawList->AddRect(min, max, ImGui::ColorConvertFloat4ToU32(ColorOf(kind, 0.7f)), size.y * 0.5f);
        drawList->AddText(ImVec2{ min.x + style.FramePadding.x, min.y + style.FramePadding.y },
            ImGui::ColorConvertFloat4ToU32(ColorOf(kind, 1.0f)), text);

        ImGui::Dummy(size);
        if (tooltip != nullptr) StatusGuiService::ShowTooltip(tooltip);
    }

    auto StatusGuiService::DrawDot(const char* label, StatusKind kind, const char* tooltip) -> void
    {
        const float height{ ImGui::GetTextLineHeight() };
        const float radius{ height * 0.28f };

        ImGui::BeginGroup();

        const ImVec2 origin{ ImGui::GetCursorScreenPos() };
        ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2{ origin.x + radius + 1.0f, origin.y + (height * 0.5f) }, radius,
            ImGui::ColorConvertFloat4ToU32(ColorOf(kind, 1.0f)));
        ImGui::Dummy(ImVec2{ (radius * 2.0f) + 2.0f, height });
        ImGui::SameLine(0.0f, 4.0f);
        ImGui::TextUnformatted(label);

        ImGui::EndGroup();

        if (tooltip != nullptr) StatusGuiService::ShowTooltip(tooltip);
    }

    auto StatusGuiService::DrawHelpMarker(const char* tooltip) -> void
    {
        ImGui::TextDisabled("(?)");
        StatusGuiService::ShowTooltip(tooltip);
    }

    auto StatusGuiService::ShowTooltip(const char* text) -> void
    {
        if (!ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip | ImGuiHoveredFlags_AllowWhenDisabled)) return;
        if (!ImGui::BeginTooltip()) return;

        ImGui::PushTextWrapPos(ImGui::GetFontSize() * k_TooltipWrapChars);
        ImGui::TextUnformatted(text);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}