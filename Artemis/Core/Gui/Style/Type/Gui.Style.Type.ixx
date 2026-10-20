module;

#include "External/imgui/imgui.h"

export module Gui.Style.Type;

import std;

export namespace Gui::Style::Type
{
    // Color of the section titles of the panels.
    inline constexpr ImVec4 k_SectionColor{ 0.8f, 0.8f, 0.8f, 1.0f };

    // Space between the cards of a card table.
    inline constexpr float k_CardSpacing{ 8.0f };

    // Seconds the feedback animation of a widget lasts.
    inline constexpr float k_AnimationDuration{ 0.6f };

    // Limits of the opacity and of the scale of the interface.
    inline constexpr float k_MinimumAlpha{ 0.20f };
    inline constexpr float k_MinimumScale{ 1.0f };
    inline constexpr float k_MaximumScale{ 4.0f };

    // Bytes per row of a hex dump.
    inline constexpr std::size_t k_BytesPerRow{ 16 };
}