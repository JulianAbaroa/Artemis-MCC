module;

#include "External/imgui/imgui.h"

module DevTools.Gui.Widget.System;
import :ResponsiveCard;

namespace DevTools::Gui::Widget::System
{
    auto ResponsiveCardGuiService::FitsOnSameLine(float nextWidth, float spacing,
        float windowRightEdge) -> bool
    {
        return ImGui::GetItemRectMax().x + spacing + nextWidth < windowRightEdge;
    }
}