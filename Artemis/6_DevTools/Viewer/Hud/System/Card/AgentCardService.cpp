module;

#include "External/imgui/imgui.h"

module DevTools.Viewer.Hud.System;
import :AgentCard;
import :Canvas;

import Common.Engine.Type;
import Common.Semantic.Type;
import Common.Team.Type;
import Blam.Runtime.Object.Type;
import Perception.Entities.Type;
import Perception.Players.Type;
import Views.View.System;
import Bridge.Codec.Tick.Type;
import DevTools.Gui.Format.System;
import DevTools.Viewer.Collidable.Type;
import DevTools.Viewer.Format.System;
import DevTools.Viewer.Inspector.System;
import DevTools.Viewer.Options.Type;
import DevTools.Viewer.Style.Type;
import std;

namespace
{
    using Common::Semantic::Type::Subtype::Powerup;
    using Common::Team::Type::Team;
    using Blam::Runtime::Object::Type::Biped::GrenadeType;
    using Blam::Runtime::Object::Type::Biped::ZoomLevel;
    using Perception::Players::Type::Loadout;
    using Perception::Players::Type::Player;
    using Bridge::Codec::Tick::Type::Tick;

    using DevTools::Gui::Format::System::TeamFormater;
    using DevTools::Viewer::Hud::System::ShortTagName;
    using DevTools::Viewer::Format::System::PlayerFormater;
    using DevTools::Viewer::Inspector::System::ObjectName;
    using DevTools::Viewer::Options::Type::Layer;
    using DevTools::Viewer::Options::Type::Level;
    using DevTools::Viewer::Options::Type::Scalar;
    using DevTools::Viewer::Style::Type::ColorOfTeam;

    using Common::Engine::Type::k_HandleIndexMask;
    using Common::Engine::Type::k_NoHandle;
    using Common::Engine::Type::k_TicksPerSecond;

    // Margin from the corner of the frame and width of a bar, in pixels at text scale 1.
    constexpr float k_Margin{ 16.0f };
    constexpr float k_BarWidth{ 180.0f };

    constexpr ImU32 k_HealthColor{ IM_COL32(70, 190, 90, 255) };
    constexpr ImU32 k_ShieldColor{ IM_COL32(70, 150, 230, 255) };
    constexpr ImU32 k_EnergyColor{ IM_COL32(230, 170, 60, 255) };

    // return: The seconds the ticks last.
    auto SecondsOf(std::uint32_t ticks) -> float
    {
        return static_cast<float>(ticks) / k_TicksPerSecond;
    }

    // return: The line of a player without a live biped.
    auto DeadText(std::uint32_t respawnTicks) -> std::string
    {
        return std::format("dead, respawn in {:.1f} s", SecondsOf(respawnTicks));
    }

    // Draws a bar from 0 to 1 with its value as text.
    auto DrawBar(const char* label, float value, ImU32 color) -> void
    {
        const float fraction{ std::clamp(value, 0.0f, 1.0f) };
        const std::string text{ std::format("{} {:.2f}", label, value) };

        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
        ImGui::ProgressBar(fraction, ImVec2(k_BarWidth * ImGui::GetIO().FontGlobalScale, 0.0f), text.c_str());
        ImGui::PopStyleColor();
    }

    // return: The short name of an object of the tick.
    auto NameOf(const Tick& tick, std::uint32_t handle) -> std::string
    {
        return ShortTagName(ObjectName::Of(tick, handle));
    }

    // Draws the grenades by type, the selected one between brackets.
    auto DrawGrenades(const Loadout& loadout) -> void
    {
        const bool isFragSelected{ loadout.SelectedGrenade == GrenadeType::Frag };
        const bool isPlasmaSelected{ loadout.SelectedGrenade == GrenadeType::Plasma };

        ImGui::Text("Grenades: %s%s %u%s  %s%s %u%s",
            isFragSelected ? "[" : "", PlayerFormater::GrenadeToString(GrenadeType::Frag), static_cast<unsigned>(loadout.FragGrenades), isFragSelected ? "]" : "",
            isPlasmaSelected ? "[" : "", PlayerFormater::GrenadeToString(GrenadeType::Plasma), static_cast<unsigned>(loadout.PlasmaGrenades), isPlasmaSelected ? "]" : "");
    }

    // Draws what the player carries besides the weapon in the hands. A row without a value is not drawn.
    auto DrawFull(const Tick& tick, const Player& player, const Loadout& loadout) -> void
    {
        const std::pair<const char*, std::uint32_t> slots[]{ { "Primary", loadout.PrimaryWeapon }, { "Secondary", loadout.SecondaryWeapon } };

        for (const auto& [label, weapon] : slots)
        {
            if (weapon == k_NoHandle || weapon == loadout.EquippedWeapon) continue;

            ImGui::Text("%s: %s", label, NameOf(tick, weapon).c_str());
        }

        DrawGrenades(loadout);

        if (loadout.Ability != k_NoHandle)
        {
            ImGui::Text("Ability: %s%s", NameOf(tick, loadout.Ability).c_str(), loadout.IsAbilityActive ? " (active)" : "");
            DrawBar("Energy", loadout.AbilityEnergy, k_EnergyColor);
        }

        if (player.Powerup != Powerup::None)
        {
            ImGui::Text("Powerup: %s %.1f s", PlayerFormater::PowerupToString(player.Powerup), SecondsOf(player.PowerupTicks));
        }

        if (loadout.Objective != k_NoHandle)
        {
            const auto* item{ tick.Entities ? tick.Entities->Find(loadout.Objective) : nullptr };

            if (item && item->Team != Team::Neutral) ImGui::Text("FLAG (%s)", TeamFormater::TeamToString(item->Team));
            else                                       ImGui::TextUnformatted("FLAG");
        }

        if (player.Body && player.Body->Zoom != ZoomLevel::None)
        {
            ImGui::Text("%s", PlayerFormater::ZoomToString(player.Body->Zoom));
        }
    }
}

namespace DevTools::Viewer::Hud::System
{
    auto AgentCardService::Draw() -> void
    {
        const Level level{ m_OptionsStore.GetLevel(Layer::Card) };
        if (level == Level::Off) return;

        // The selection follows the player (VW.9a B6): dead or respawning, the card stays on it.
        const std::uint32_t playerHandle{ m_SelectionStore.GetSelectedPlayer() };
        if (playerHandle == k_NoHandle) return;

        const auto scene{ m_SceneService.GetFrameScene(m_CollidableStore) };
        if (!scene || !scene->Tick || !scene->Tick->Players) return;

        const Tick& tick{ *scene->Tick };

        const Player* player{ tick.Players->Find(playerHandle) };
        if (!player) return;

        m_BotStore.CopyBots(m_Bots);
        const std::uint32_t playerIndex{ player->Handle & k_HandleIndexMask };
        const bool isBot{ std::ranges::any_of(m_Bots, [playerIndex](const auto& bot) { return bot.PlayerIndex == playerIndex; }) };

        const ImGuiViewport* viewport{ ImGui::GetMainViewport() };
        const float scale{ m_OptionsStore.GetScalar(Scalar::LabelTextScale) };
        const float margin{ k_Margin * scale };

        ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + margin, viewport->WorkPos.y + viewport->WorkSize.y - margin), ImGuiCond_Always, ImVec2(0.0f, 1.0f));
        ImGui::SetNextWindowBgAlpha(m_OptionsStore.GetScalar(Scalar::LabelBackgroundAlpha));

        constexpr ImGuiWindowFlags flags{ ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav |
            ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings };

        if (ImGui::Begin("##AgentCard", nullptr, flags))
        {
            ImGui::SetWindowFontScale(scale);

            const ImVec4 teamColor{ ImGui::ColorConvertU32ToFloat4(ToImColor(ColorOfTeam(player->Team))) };
            ImGui::TextColored(teamColor, "%s%s", player->Name.c_str(), isBot ? " (bot)" : "");

            if (!player->Body)
            {
                ImGui::TextUnformatted(DeadText(player->RespawnTicks).c_str());
            }
            else
            {
                const auto* entity{ tick.Entities ? tick.Entities->Find(player->BipedHandle) : nullptr };

                if (entity && entity->Vitality) DrawBar("Health", *entity->Vitality, k_HealthColor);
                if (player->Body->Shield) DrawBar("Shield", *player->Body->Shield, k_ShieldColor);

                const Loadout& loadout{ player->Body->Loadout };

                if (loadout.EquippedWeapon != k_NoHandle) ImGui::Text("Weapon: %s", NameOf(tick, loadout.EquippedWeapon).c_str());

                if (level == Level::Full) DrawFull(tick, *player, loadout);
            }
        }

        ImGui::End();
    }
}