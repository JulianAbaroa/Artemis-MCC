module;

#include "External/imgui/imgui.h"

module Gui.Widget.System;
import :HandleDrawer;

import Common.Engine.Type;
import Gui.Format.System;
import std;

namespace
{
    using Gui::Format::System::HexFormater;

    using Common::Engine::Type::k_NoHandle;
}

namespace Gui::Widget::System
{
    auto HandleDrawerGuiService::DrawU32(const char* label, std::uint32_t handle,
        std::uint32_t ownerHandle, CopyableFieldGuiService& copyableField) -> void
    {
        if (handle == k_NoHandle)
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