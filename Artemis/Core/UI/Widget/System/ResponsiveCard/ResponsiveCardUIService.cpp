module;

#include "External/imgui/imgui.h"

module UI.Widget.System;
import :ResponsiveCard;

namespace UI::Widget::System
{
	auto ResponsiveCardUIService::FitsOnSameLine(float nextWidth, float spacing,
		float windowRightEdge) -> bool
	{
		return ImGui::GetItemRectMax().x + spacing + nextWidth < windowRightEdge;
	}
}