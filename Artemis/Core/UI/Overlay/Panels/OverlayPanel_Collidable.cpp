#include "pch.h"

#include "OverlayPanel_Collidable.h"

#include "Core/Types/Tick/Tick.h"
#include "Core/Types/Environment/Collidable/Collidable.h"

#include "Core/UI/Utils/Hex/HexFormater.h"

#include "External/imgui/imgui.h"

namespace
{
    void DrawCollidable(const Collidable& instance)
    {
        ImGui::TextColored(ImVec4(0.4f, 0.86f, 1.0f, 1.0f), "Collidable");

        ImGui::Separator();

        ImGui::Text("%s", instance.TagName.c_str());
        ImGui::Text("Handle: %s", HexFormater::Hex32(instance.Handle).c_str());

        ImGui::Spacing();

        ImGui::Text("Position: %.2f, %.2f, %.2f", instance.Position[0],
            instance.Position[1], instance.Position[2]);

        ImGui::Text("Forward: %.2f, %.2f, %.2f", instance.Forward[0],
            instance.Forward[1], instance.Forward[2]);

        ImGui::Text("Up: %.2f, %.2f, %.2f", instance.Up[0],
            instance.Up[1], instance.Up[2]);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (instance.Coll)
        {
            const auto& coll = *instance.Coll;

            ImGui::TextDisabled("Coll geometry:");
            ImGui::Text("%s", coll.TagName.c_str());

            ImGui::Spacing();

            ImGui::Text("Meshes: %d", static_cast<int>(coll.Meshes.size()));
            ImGui::Text("Nodes: %d", static_cast<int>(coll.Nodes.size()));

            ImGui::Spacing();

            ImGui::TextDisabled("Bounds (model-space):");
            ImGui::Text("Min: %.2f, %.2f, %.2f",
                coll.BoundsMin.X, coll.BoundsMin.Y, coll.BoundsMin.Z);
            ImGui::Text("Max: %.2f, %.2f, %.2f",
                coll.BoundsMax.X, coll.BoundsMax.Y, coll.BoundsMax.Z);
        }
        else
        {
            ImGui::TextDisabled("Coll geometry: none");
        }

        ImGui::Spacing();

        ImGui::TextDisabled("Collidable mesh:");
        ImGui::Text("Triangles: %d",
            static_cast<int>(instance.CollidableMesh.Triangles.size()));
    }
}

void OverlayPanel_Collidable::Draw(
    const std::shared_ptr<const Tick>& tick, uint32_t handle)
{
    if (!tick || !tick->Collidables)
    {
        ImGui::TextDisabled("No collidable data.");
        return;
    }

    for (const auto& inst : *tick->Collidables)
    {
        if (inst.Handle == handle)
        {
            DrawCollidable(inst);
            return;
        }
    }

    ImGui::TextDisabled("Selected object is not a collidable.");
}