module;

#include "External/imgui/imgui.h"

export module Viewer.Hud.Type;

import Common.Math.Type;
import Relations.Classifier.Type;
import Export.Tick.Type;
import Viewer.Options.State;
import std;

export namespace Viewer::Hud::Type
{
    // Key of a label that is not merged with any other.
    inline constexpr std::uint32_t k_NoLabelKey{ 0xFFFFFFFF };

    // One line of a label. A divider draws a rule and ignores the text.
    struct LabelRow
    {
        std::string Text{};
        ImU32 Color{ IM_COL32(255, 255, 255, 255) };
        bool IsDivider{ false };
    };

    // What the label collectors read in a frame.
    // Selected is the selected handle, or the no-selection handle.
    // The self fields are valid only if HasSelf is true, except SelfBiped which is set whenever the local player exists.
    struct LabelContext
    {
        const Export::Tick::Type::Tick& Frame;
        const Viewer::Options::State::OptionsStore& Options;
        const std::unordered_map<std::uint32_t, Relations::Classifier::Type::Role>& Roles;

        std::uint32_t Selected{ 0xFFFFFFFF };

        bool HasSelf{ false };
        Common::Math::Type::Vec3 SelfPosition{};
        std::uint32_t SelfBiped{ 0xFFFFFFFF };
    };

    // Label already projected to the screen. Rows are drawn from top to bottom.
    struct Entry
    {
        ImVec2 Screen{};
        float Distance{};
        float Priority{};
        ImU32 Accent{};
        std::vector<LabelRow> Rows{};

        ImVec2 BoxMin{};
        ImVec2 BoxMax{};
    };

    // Health bar already projected to the screen. Fraction goes from 0 to 1.
    struct Bar
    {
        ImVec2 Screen{};
        float Distance{};
        float Priority{};
        float Fraction{};
        ImU32 Color{};
        std::string Text{};
    };

    // Arrow already projected to the screen.
    struct Arrow
    {
        ImVec2 From{};
        ImVec2 To{};
        ImU32 Color{};
        float Thickness{};
        bool Dashed{ false };
    };
}