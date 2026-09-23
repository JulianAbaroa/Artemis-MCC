module;

#include "External/imgui/imgui.h"

module UI.Widget.System;
import :HandleDrawer;

import UI.Format.System;

namespace
{
    using HexFormater = UI::Format::System::HexFormater;

    constexpr std::uint32_t k_InvalidHandle = 0xFFFFFFFF;
}

namespace UI::Widget::System
{
    auto HandleDrawerUIService::DrawU32(const char* label, std::uint32_t handle,
        std::uint32_t ownerHandle, CopyableFieldUIService& copyableField) -> void
    {
        if (handle == k_InvalidHandle)
        {
            ImGui::TextDisabled("%s", label);
            ImGui::SameLine();
            ImGui::TextDisabled("none");
        }
        else
        {
            copyableField.Draw(label, HexFormater::Hex32(handle), ownerHandle);
        }
    }
}