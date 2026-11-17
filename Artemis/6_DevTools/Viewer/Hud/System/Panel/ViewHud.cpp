module;

#include "External/imgui/imgui.h"

module DevTools.Viewer.Hud.System;
import :ObservedView;

import Common.Engine.Type;
import Common.Math.Type;
import Common.Math.System;
import Common.Semantic.Type;
import Views.View.Type;
import DevTools.Viewer.Style.Type;
import std;

namespace
{
    namespace VecMath = Common::Math::System;
    namespace SemanticType = Common::Semantic::Type;

    using Views::View::Type::View;
    using Views::View::Type::ViewChannel;
    using Views::View::Type::ViewFlag;
    using Views::View::Type::ViewMode;
    using Views::View::Type::ViewOther;
    using Views::View::Type::ViewRelation;
    using Common::Semantic::Type::FlagState;
    using DevTools::Viewer::Style::Type::Color;

    using Common::Engine::Type::k_MaxPlayers;
    using DevTools::Viewer::Style::Type::k_ViewAlly;
    using DevTools::Viewer::Style::Type::k_ViewEnemy;
    using DevTools::Viewer::Style::Type::k_ViewHidden;
    using DevTools::Viewer::Style::Type::k_ViewNone;

    auto RelationName(ViewRelation relation) -> const char*
    {
        switch (relation)
        {
        case ViewRelation::Self:    return "Self";
        case ViewRelation::Ally:    return "Ally";
        case ViewRelation::Enemy:   return "Enemy";
        case ViewRelation::Neutral: return "Neutral";
        default:                    return "None";
        }
    }

    auto RelationColor(ViewRelation relation) -> ImVec4
    {
        const Color color{ relation == ViewRelation::Enemy ? k_ViewEnemy
            : relation == ViewRelation::Ally ? k_ViewAlly
            : relation == ViewRelation::Self ? k_ViewHidden : k_ViewNone };

        return ImVec4(color.R, color.G, color.B, 1.0f);
    }

    auto ChannelsText(std::uint8_t channels) -> std::string
    {
        std::string text{};
        text += (channels & static_cast<std::uint8_t>(ViewChannel::Hud)) ? 'H' : '-';
        text += (channels & static_cast<std::uint8_t>(ViewChannel::Seen)) ? 'S' : '-';
        text += (channels & static_cast<std::uint8_t>(ViewChannel::Radar)) ? 'R' : '-';
        return text;
    }

    auto DrawOthers(const View& view) -> void
    {
        ImGui::TextDisabled("Others (row, relation, channels Hud/Seen/Radar, distance):");

        const Common::Math::Type::Vec3 eye{ view.Self.Eye };
        std::size_t listed{ 0 };

        for (std::size_t row{ 0 }; row < view.Others.size(); ++row)
        {
            const ViewOther& other{ view.Others[row] };
            if (other.Channels == 0 && other.Relation == ViewRelation::None) continue;

            ++listed;

            const std::string channels{ ChannelsText(other.Channels) };
            const bool hasPosition{ other.Channels != 0 && other.IsAlive };

            if (hasPosition)
            {
                ImGui::TextColored(RelationColor(other.Relation), "%2d %-7s %s %6.1f wu%s", static_cast<int>(row),
                    RelationName(other.Relation), channels.c_str(), VecMath::Distance(eye, other.Position),
                    other.CarriesFlag ? " flag" : "");
            }
            else
            {
                ImGui::TextColored(RelationColor(other.Relation), "%2d %-7s %s      -", static_cast<int>(row),
                    RelationName(other.Relation), channels.c_str());
            }
        }

        if (listed == 0) ImGui::TextDisabled("none");
    }

    auto DrawFlag(const char* label, const ViewFlag& flag) -> void
    {
        const std::string_view state{ SemanticType::NameOf(flag.State) };

        ImGui::Text("%s: %.*s", label, static_cast<int>(state.size()), state.data());
        ImGui::SameLine();

        if (flag.State == FlagState::Carried && flag.CarrierRow < k_MaxPlayers) ImGui::Text("carrier %d", static_cast<int>(flag.CarrierRow));
        else                                                                    ImGui::TextDisabled("no carrier");

        ImGui::SameLine();
        ImGui::Text("reset %u%s", static_cast<unsigned>(flag.ResetTicks), flag.IsResetRunning ? " (running)" : "");
    }
}

namespace DevTools::Viewer::Hud::System
{
    auto ViewHud::Draw(const View* view) -> void
    {
        if (!view)
        {
            ImGui::TextDisabled("No View.");
            return;
        }

        const bool isFull{ view->Head.Mode == ViewMode::Full };

        ImGui::Text("Row %d | Mode %s | Side %u | Team %u", static_cast<int>(view->Head.PlayerRow),
            isFull ? "Full" : "HumanLike", static_cast<unsigned>(view->Head.Side), static_cast<unsigned>(view->Head.Team));

        ImGui::Separator();
        DrawOthers(*view);

        ImGui::Separator();
        DrawFlag("Own flag", view->Flags[0]);
        DrawFlag("Rival flag", view->Flags[1]);
    }
}