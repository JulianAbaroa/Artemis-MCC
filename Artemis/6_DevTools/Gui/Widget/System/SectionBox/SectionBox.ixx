module;

#include "External/imgui/imgui.h"

export module DevTools.Gui.Widget.System:SectionBox;

import std;

export namespace DevTools::Gui::Widget::System
{
    // Draws a rounded box around the widgets created while it lives.
    class SectionBox
    {
    public:
        // param useFullWidth: Stretches the box to the available width instead of the content width.
        explicit SectionBox(bool useFullWidth = false);
        ~SectionBox();

        SectionBox(const SectionBox&) = delete;
        auto operator=(const SectionBox&) -> SectionBox& = delete;

    private:
        bool m_IsFullWidth{ false };
        ImVec2 m_Min{};
        float m_Width{ 0.0f };
    };
}