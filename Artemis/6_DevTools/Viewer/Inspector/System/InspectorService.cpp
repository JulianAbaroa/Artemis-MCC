module;

#include "External/imgui/imgui.h"

module DevTools.Viewer.Inspector.System;

import Common.Engine.Type;
import Perception.Entities.Type;
import Perception.Players.Type;
import Views.View.Type;
import Views.View.System;
import Bridge.Codec.Tick.Type;
import DevTools.Gui.Format.System;
import std;

namespace
{
    using Bridge::Codec::Tick::Type::Tick;
    using Perception::Entities::Type::Entity;
    using Perception::Players::Type::Player;
    using Views::View::Type::View;
    using DevTools::Viewer::Inspector::System::ObjectName;

    using Views::View::System::FindObservedView;
    using Views::View::System::ObservedPlayerOf;

    using DevTools::Gui::Format::System::HexFormater;
    using DevTools::Gui::Format::System::TeamFormater;

    using Common::Engine::Type::k_NoHandle;
    using Common::Engine::Type::k_PrimaryLocalUser;

    // Draws the always visible lines: name, handle, team, vitality and shield.
    auto DrawSummary(const Tick& tick, std::uint32_t handle, const Entity* entity, const Player* player) -> void
    {
        ImGui::TextWrapped("%s", (player ? player->Name : ObjectName::Of(tick, handle)).c_str());
        ImGui::TextDisabled("%s", HexFormater::Hex32(handle).c_str());

        std::string line{};

        if (player) line += std::format("Team {}", TeamFormater::TeamToString(player->Team));
        else if (entity) line += std::format("Team {}", TeamFormater::TeamToString(entity->Team));

        if (entity && entity->Vitality) line += std::format("{}Vitality {:.2f}", line.empty() ? "" : " | ", *entity->Vitality);

        if (player && player->Body && player->Body->Shield) line += std::format("{}Shield {:.2f}", line.empty() ? "" : " | ", *player->Body->Shield);

        if (!line.empty()) ImGui::TextUnformatted(line.c_str());
    }
}

namespace DevTools::Viewer::Inspector::System
{
    auto InspectorService::Draw() -> void
    {
        const std::uint32_t handle{ m_SelectionStore.GetSelected() };
        if (handle == k_NoHandle)
        {
            ImGui::TextDisabled("Nothing selected");
            return;
        }

        const auto scene{ m_SceneService.GetFrameScene(m_CollidableStore) };
        if (!scene || !scene->Tick)
        {
            ImGui::TextDisabled("No data");
            return;
        }

        const Tick& tick{ *scene->Tick };

        const Entity* entity{ EntitySection::Find(tick, handle) };

        const std::uint32_t playerHandle{ tick.Players ? ObservedPlayerOf(*tick.Players, handle) : k_NoHandle };
        const Player* player{ playerHandle != k_NoHandle ? tick.Players->Find(playerHandle) : nullptr };
        const Perception::Players::Type::Body* body{ player && player->Body ? &*player->Body : nullptr };
        const bool isLocalBiped{ player && player->LocalIndex == k_PrimaryLocalUser };

        // note: FindObservedView falls back to the local player, so the View counts only when it is the one of that player.
        const View* view{ player && tick.Views ? FindObservedView(*tick.Views, playerHandle) : nullptr };
        if (view && view->Head.PlayerHandle != playerHandle) view = nullptr;

        const auto* affordance{ InteractionSection::Find(tick, handle) };
        const auto* vitality{ VitalitySection::Find(tick, handle) };
        const auto* collidable{ CollisionSection::Find(scene->Objects, handle) };

        // The ids are the names of the sections, so a section keeps its folded state when the selection changes.
        ImGui::PushID("Inspector");

        DrawSummary(tick, handle, entity, player);
        ImGui::Spacing();

        bool isFirst{ true };
        const auto drawSection = [&isFirst](const char* title, const auto& drawBody)
        {
            const ImGuiTreeNodeFlags flags{ isFirst ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_None };
            isFirst = false;

            if (ImGui::CollapsingHeader(title, flags)) drawBody();
        };

        if (view) drawSection("View", [&] { ViewSection::Draw(view); });
        if (player) drawSection("Player", [&] { PlayerSection::Draw(tick, *player); });
        if (body) drawSection("Body", [&] { BodySection::Draw(tick, *body); });
        if (body) drawSection("Loadout", [&] { LoadoutSection::Draw(tick, body->Loadout); });
        if (player) drawSection("Action", [&] { ActionSection::Draw(*player, m_PlayerActionStore, m_BotStore); });
        if (affordance || isLocalBiped) drawSection("Interaction", [&] { InteractionSection::Draw(tick, affordance); });
        if (vitality) drawSection("Vitality", [&] { VitalitySection::Draw(tick, handle, *vitality); });
        if (entity) drawSection("Entity", [&] { EntitySection::Draw(*entity); });
        if (FixtureSection::Applies(tick, handle)) drawSection("Fixture", [&] { FixtureSection::Draw(tick, handle); });
        if (collidable) drawSection("Collision", [&] { CollisionSection::Draw(*collidable); });
        if (collidable || isLocalBiped) drawSection("Raw", [&] { RawSection::Draw(tick, collidable, isLocalBiped); });

        ImGui::PopID();
    }
}