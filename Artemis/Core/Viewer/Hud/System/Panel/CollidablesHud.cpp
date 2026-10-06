module;

#include "External/imgui/imgui.h"

module Viewer.Hud.System;
import :Collidables;

import Gui.Widget.System;
import std;

namespace
{
    using Tick = Export::Tick::Type::Tick;
    using Collidable = Export::Tick::Type::Collidable;

    using PanelGuiService = Gui::Widget::System::PanelGuiService;

    constexpr ImVec4 k_AlertColor{ 1.0f, 0.4f, 0.4f, 1.0f };

    auto DrawCollidable(const Collidable& instance) -> void
    {
        PanelGuiService::DrawHeader(ImVec4(0.4f, 0.86f, 1.0f, 1.0f), "Collidable",
            instance.TagName, instance.Handle);

        ImGui::Spacing();

        PanelGuiService::DrawVec3("Position:", instance.Position);
        PanelGuiService::DrawVec3("Forward:", instance.Forward);
        PanelGuiService::DrawVec3("Up:", instance.Up);

        PanelGuiService::DrawSectionSeparator();

        const auto& mesh = instance.WorldMesh;

        ImGui::TextDisabled("World mesh:");
        ImGui::Text("Triangles: %d", static_cast<int>(mesh.Triangles.size()));
        ImGui::Text("Node: %d", static_cast<int>(mesh.NodeIndex));
        ImGui::Text("Region: %d", static_cast<int>(mesh.RegionIndex));
        ImGui::Text("Permutation: %d", static_cast<int>(mesh.PermutationIndex));

        ImGui::Spacing();

        ImGui::TextDisabled("Bounds (model-space):");
        PanelGuiService::DrawVec3("Min:", mesh.LocalMin);
        PanelGuiService::DrawVec3("Max:", mesh.LocalMax);

        PanelGuiService::DrawSectionSeparator();

        PanelGuiService::DrawBoolBadge("Ancestor dead:", instance.AncestorDead, k_AlertColor);
        PanelGuiService::DrawBoolBadge("Destroyed geometry:", instance.HasDestroyedGeometry, k_AlertColor);
    }
}

namespace Viewer::Hud::System
{
    auto CollidablesHud::Draw(const Tick& tick, std::uint32_t handle) -> void
    {
        if (!tick.Collidables)
        {
            ImGui::TextDisabled("No collidable data.");
            return;
        }

        for (const Collidable& instance : *tick.Collidables)
        {
            if (instance.Handle == handle)
            {
                DrawCollidable(instance);
                return;
            }
        }

        ImGui::TextDisabled("Selected object is not a collidable.");
    }
}