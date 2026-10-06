module;

#include "External/imgui/imgui.h"

module Gui.Widget.System;
import :HandleDrawer;

import Gui.Format.System;
import std;

namespace
{
    using HexFormater = Gui::Format::System::HexFormater;

    constexpr std::uint32_t k_InvalidHandle{ 0xFFFFFFFF };
}

namespace Gui::Widget::System
{
    auto HandleDrawerGuiService::DrawU32(const char* label, std::uint32_t handle,
        std::uint32_t ownerHandle, CopyableFieldGuiService& copyableField) -> void
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