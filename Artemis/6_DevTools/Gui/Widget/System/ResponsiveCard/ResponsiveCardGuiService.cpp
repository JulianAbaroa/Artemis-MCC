module;

#include "External/imgui/imgui.h"

module Gui.Widget.System;
import :ResponsiveCard;

namespace Gui::Widget::System
{
    auto ResponsiveCardGuiService::FitsOnSameLine(float nextWidth, float spacing,
        float windowRightEdge) -> bool
    {
        return ImGui::GetItemRectMax().x + spacing + nextWidth < windowRightEdge;
    }
}