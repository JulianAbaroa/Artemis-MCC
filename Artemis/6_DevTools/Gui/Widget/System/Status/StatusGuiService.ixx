module;

#include "External/imgui/imgui.h"

export module DevTools.Gui.Widget.System:Status;

import DevTools.Gui.Style.Type;
import std;

export namespace DevTools::Gui::Widget::System
{
    // Draws the state markers of the tools: chips, dots, the "(?)" marker and the tooltip of a control.
    class StatusGuiService
    {
    private:
        using StatusKind = DevTools::Gui::Style::Type::StatusKind;

    public:
        StatusGuiService() = default;
        ~StatusGuiService() = default;

        // Draws the text in a rounded box tinted with the color of the kind.
        // param tooltip: Shown while the chip is hovered. Null for none.
        static auto DrawChip(const char* text, StatusKind kind, const char* tooltip = nullptr) -> void;

        // Draws a dot in the color of the kind followed by the label, as one item.
        // param tooltip: Shown while the dot or the label is hovered. Null for none.
        static auto DrawDot(const char* label, StatusKind kind, const char* tooltip = nullptr) -> void;

        // Draws a "(?)" marker that shows the tooltip while hovered.
        static auto DrawHelpMarker(const char* tooltip) -> void;

        // Shows the tooltip of the last item while it is hovered, also when the item is disabled.
        // note: A tooltip of a disabled control says why it cannot act.
        static auto ShowTooltip(const char* text) -> void;
    };
}