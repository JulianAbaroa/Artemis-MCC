module;

#include "External/imgui/imgui.h"

module DevTools.Viewer.Inspector.System;
import :Collision;

import Perception.Collidable.Type;
import DevTools.Gui.Widget.System;
import std;

namespace
{
    using Perception::Collidable::Type::Collidable;

    using DevTools::Gui::Widget::System::PanelGuiService;
}

namespace DevTools::Viewer::Inspector::System
{
    auto CollisionSection::Find(const Collidables& collidables, std::uint32_t handle) -> const Collidable*
    {
        for (const Collidable& instance : collidables)
        {
            if (instance.Handle == handle) return &instance;
        }

        return nullptr;
    }

    auto CollisionSection::Draw(const Collidable& instance) -> void
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
    }
}