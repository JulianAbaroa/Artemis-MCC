module;

#include "External/imgui/imgui.h"

module Gui.Widget.System;
import :CopyableField;

import std;

namespace Gui::Widget::System
{
    auto CopyableFieldGuiService::Draw(const char* label, const std::string& value,
        std::uint32_t ownerHandle) -> void
    {
        const std::string uniqueId = std::to_string(ownerHandle) + "_" + label;

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));

        std::vector<char> buffer(value.begin(), value.end());
        buffer.push_back('\0');

        ImGui::TextDisabled("%s", label);
        ImGui::SameLine();

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::InputText(
            ("##copy_" + uniqueId).c_str(),
            buffer.data(),
            buffer.size(),
            ImGuiInputTextFlags_ReadOnly
        );

        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Right-click to copy.");
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            {
                ImGui::SetClipboardText(value.c_str());
                m_AnimateCopyLabel = uniqueId;
                m_AnimationStartTime = static_cast<float>(ImGui::GetTime());
            }
        }

        if (m_AnimateCopyLabel == uniqueId)
        {
            const float elapsed = static_cast<float>(ImGui::GetTime()) - m_AnimationStartTime;
            if (elapsed < k_AnimationDuration)
            {
                const float alpha = 1.0f - (elapsed / k_AnimationDuration);
                ImGui::GetWindowDrawList()->AddRectFilled(
                    ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                    ImColor(0.8f, 0.8f, 0.8f, alpha * 0.4f),
                    ImGui::GetStyle().FrameRounding
                );
            }
            else
            {
                m_AnimateCopyLabel.clear();
            }
        }
    }
}