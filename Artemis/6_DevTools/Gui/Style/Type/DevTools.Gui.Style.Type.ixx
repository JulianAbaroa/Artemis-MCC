module;

#include "External/imgui/imgui.h"

export module DevTools.Gui.Style.Type;

import std;

export namespace DevTools::Gui::Style::Type
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

    // What a status marker says about its subject.
    enum class StatusKind : std::uint8_t
    {
        // Inactive, nothing to report.
        Off = 0,
        // Running or connected.
        Active,
        // Requested, waiting to be applied.
        Pending,
        // Done.
        Ok,
        // Failed, or recording.
        Error,

        Count
    };

    inline constexpr std::size_t k_StatusKindCount{ static_cast<std::size_t>(StatusKind::Count) };

    // Color of each StatusKind, in its order.
    inline constexpr std::array<ImVec4, k_StatusKindCount> k_StatusColors
    {
        ImVec4{ 0.55f, 0.55f, 0.55f, 1.0f },
        ImVec4{ 0.30f, 0.90f, 0.35f, 1.0f },
        ImVec4{ 1.00f, 0.75f, 0.20f, 1.0f },
        ImVec4{ 0.40f, 0.75f, 1.00f, 1.0f },
        ImVec4{ 1.00f, 0.35f, 0.35f, 1.0f },
    };

    // Characters (in font sizes) after which a tooltip wraps.
    inline constexpr float k_TooltipWrapChars{ 32.0f };

    // Bytes per row of a hex dump.
    inline constexpr std::size_t k_BytesPerRow{ 16 };
}