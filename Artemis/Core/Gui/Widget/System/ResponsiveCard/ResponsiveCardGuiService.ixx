module;

#include "External/imgui/imgui.h"

export module Gui.Widget.System:ResponsiveCard;

import std;

export namespace Gui::Widget::System
{
    // Draws content inside a rounded card of fixed size.
    class ResponsiveCardGuiService
    {
    public:
        ResponsiveCardGuiService() = default;
        ~ResponsiveCardGuiService() = default;

        // Runs drawContent inside a child window with no scrollbar.
        // param id: Makes the ImGui id of the card unique.
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

        // Tells if a card of nextWidth fits on the line of the last item, so the cards wrap with the window.
        // param windowRightEdge: Right edge of the window in screen coordinates.
        // note: Reads the last item, so call it right after drawing a card.
        static auto FitsOnSameLine(float nextWidth, float spacing, float windowRightEdge) -> bool;
    };
}