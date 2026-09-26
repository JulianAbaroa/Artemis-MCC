module;

#include "External/imgui/imgui.h"

export module UI.Widget.System:ResponsiveCard;

import std;

export namespace UI::Widget::System
{
	class ResponsiveCardUIService
	{
	public:
		ResponsiveCardUIService() = default;
		~ResponsiveCardUIService() = default;

		template <typename Drawer>
		static auto Draw(std::uint32_t id, const ImVec2& cardSize, Drawer drawContent) -> void
		{
			ImGui::PushID(static_cast<int>(id));
			ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f);

			constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar |
				ImGuiWindowFlags_NoScrollWithMouse;

			if (ImGui::BeginChild("##card", cardSize, true, flags))
			{
				drawContent();
			}

			ImGui::EndChild();
			ImGui::PopStyleVar();
			ImGui::PopID();
		}

		static auto FitsOnSameLine(float nextWidth, float spacing, float windowRightEdge) -> bool;
	};
}