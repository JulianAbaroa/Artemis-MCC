module;

#include "External/imgui/imgui.h"

export module DevTools.Viewer.Hud.Type;

import Common.Engine.Type;
import Common.Math.Type;
import Perception.Classifier.Type;
import Bridge.Codec.Tick.Type;
import std;

namespace
{
    using Common::Engine::Type::k_NoHandle;
}

export namespace DevTools::Viewer::Hud::Type
{
    // Colors of the rows of a label: secondary information, muted information, free and taken seats.
    inline constexpr ImU32 k_InfoColor{ IM_COL32(190, 200, 215, 255) };
    inline constexpr ImU32 k_MutedColor{ IM_COL32(150, 160, 175, 255) };
    inline constexpr ImU32 k_FreeSeatColor{ IM_COL32(120, 200, 130, 255) };
    inline constexpr ImU32 k_TakenSeatColor{ IM_COL32(215, 110, 110, 255) };

    // Priority of the label of the selected object, so it survives the label limit.
    inline constexpr float k_SelectedPriority{ 1.0f };

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
        const Bridge::Codec::Tick::Type::Tick& Frame;
        // Classification of each live object, by handle. The pointers live as long as Frame.
        const std::unordered_map<std::uint32_t, const Perception::Classifier::Type::Classified*>& Classifications;

        std::uint32_t Selected{ k_NoHandle };

        bool HasSelf{ false };
        Common::Math::Type::Vec3 SelfPosition{};
        std::uint32_t SelfBiped{ k_NoHandle };
    };

    // Label already projected to the screen. Rows are drawn from top to bottom.
    // Group is the semantic group color and Team is the team color. Both fill the top bar.
    struct Entry
    {
        ImVec2 Screen{};
        float Distance{};
        float Priority{};
        ImU32 Group{};
        ImU32 Team{};
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