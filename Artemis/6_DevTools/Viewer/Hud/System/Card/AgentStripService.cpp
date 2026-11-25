module;

#include "External/imgui/imgui.h"

module DevTools.Viewer.Hud.System;
import :AgentStrip;
import :Canvas;

import Common.Engine.Type;
import DevTools.Viewer.Options.Type;
import DevTools.Viewer.Roster.Type;
import DevTools.Viewer.Roster.System;
import DevTools.Viewer.Style.Type;
import std;

namespace
{
    using DevTools::Viewer::Hud::System::ToImColor;
    using DevTools::Viewer::Options::Type::Flag;
    using DevTools::Viewer::Options::Type::Scalar;
    using DevTools::Viewer::Roster::System::IndexOfPlayer;
    using DevTools::Viewer::Roster::System::StripOf;
    using DevTools::Viewer::Roster::Type::RosterEntry;
    using DevTools::Viewer::Style::Type::ColorOfTeam;

    using Common::Engine::Type::k_NoHandle;

    // Margin from the top of the frame, in pixels at text scale 1.
    constexpr float k_StripMargin{ 16.0f };

    // Opacity of the neighbours.
    constexpr float k_DimAlpha{ 0.55f };

    // Draws the name of a player in its team color, with (dead) when it has no body. The current one goes between brackets.
    auto DrawName(const RosterEntry& entry, bool isCurrent) -> void
    {
        const ImVec4 color{ ImGui::ColorConvertU32ToFloat4(ToImColor(ColorOfTeam(entry.Team), isCurrent ? 1.0f : k_DimAlpha)) };

        ImGui::TextColored(color, isCurrent ? "[%s]%s" : "%s%s", entry.Name.c_str(), entry.IsAlive ? "" : " (dead)");
    }
}

namespace DevTools::Viewer::Hud::System
{
    auto AgentStripService::Draw() -> void
    {
        if (!m_OptionsStore.IsEnabled(Flag::AgentStrip)) return;

        const std::uint32_t playerHandle{ m_SelectionStore.GetSelectedPlayer() };
        if (playerHandle == k_NoHandle) return;

        const std::vector<RosterEntry>& roster{ m_SceneService.GetRoster() };
        const std::optional<std::size_t> current{ IndexOfPlayer(roster, playerHandle) };
        if (!current) return;

        const auto strip{ StripOf(roster.size(), *current) };

        const ImGuiViewport* viewport{ ImGui::GetMainViewport() };
        const float scale{ m_OptionsStore.GetScalar(Scalar::LabelTextScale) };

        ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x * 0.5f, viewport->WorkPos.y + k_StripMargin * scale), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
        ImGui::SetNextWindowBgAlpha(m_OptionsStore.GetScalar(Scalar::LabelBackgroundAlpha));

        constexpr ImGuiWindowFlags flags{ ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav |
            ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings };

        if (ImGui::Begin("##AgentStrip", nullptr, flags))
        {
            ImGui::SetWindowFontScale(scale);

            if (strip.Previous)
            {
                ImGui::TextDisabled("<");
                ImGui::SameLine();
                DrawName(roster[*strip.Previous], false);
                ImGui::SameLine();
            }

            DrawName(roster[strip.Current], true);

            if (strip.Next)
            {
                ImGui::SameLine();
                DrawName(roster[*strip.Next], false);
                ImGui::SameLine();
                ImGui::TextDisabled(">");
            }

            // note: The position keeps the scale of the names: a smaller font sat high on the line and was hard to read.
            ImGui::SameLine();
            ImGui::TextDisabled("%zu / %zu", strip.Current + 1, roster.size());
        }

        ImGui::End();
    }
}