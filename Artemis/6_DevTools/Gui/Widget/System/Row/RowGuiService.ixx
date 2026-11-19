module;

#include "External/imgui/imgui.h"

export module DevTools.Gui.Widget.System:Row;

import std;

export namespace DevTools::Gui::Widget::System
{
    // Lays out the rows of a settings panel: a label at the left and the item that fills the rest of the row.
    class RowGuiService
    {
    public:
        RowGuiService() = default;
        ~RowGuiService() = default;

        // Draws the label at the left of a row and sizes the next item to fill the rest of it, so the row follows the width of the window.
        // A row with too little room puts the item on the line below its label.
        // param tooltip: Shown while the label is hovered. Null for none.
        static auto LabelBeforeItem(const char* label, const char* tooltip) -> void;
    };
}