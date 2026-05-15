#include "pch.h"

// Header.
#include "UI_ObjectGraph.h"

// Types.
#include "Core/Types/Domain/Graph/ObjectNode.h"
#include "Core/Types/Domain/Graph/PlayerTree.h"

// --- States ---

#include "Core/States/Domain/Object/State_ObjectTable.h"
#include "Core/States/Domain/Player/State_PlayerTable.h"
#include "Core/States/Domain/Graph/State_ObjectGraph.h"
#include "Core/States/Domain/Graph/State_PlayerGraph.h"

// --- Systems ---

#include "Core/Systems/Interface/Debug/System_Debug.h"

// ImGui.
#include "External/imgui/imgui.h"

#include <Algorithm>
#include <format>

void UI_ObjectGraph::Draw()
{
    const auto& nodes = m_Deps.State_ObjectGraph.GetNodes();
    const auto& playerTrees = m_Deps.State_PlayerGraph.GetTrees();

    auto it_remove = std::remove_if(m_DiscoveryOrder.begin(), m_DiscoveryOrder.end(),
        [&](uint32_t h) { return nodes.find(h) == nodes.end(); });
    m_DiscoveryOrder.erase(it_remove, m_DiscoveryOrder.end());

    for (const auto& [handle, node] : nodes)
    {
        if (std::find(m_DiscoveryOrder.begin(), m_DiscoveryOrder.end(), handle)
            == m_DiscoveryOrder.end())
            m_DiscoveryOrder.push_back(handle);
    }

    std::vector<uint32_t> roots;
    roots.reserve(m_DiscoveryOrder.size());
    for (uint32_t h : m_DiscoveryOrder)
    {
        auto it = nodes.find(h);
        if (it != nodes.end() && it->second.ParentHandle == 0xFFFFFFFF)
            roots.push_back(h);
    }

    ImGui::TextDisabled("Nodes: %zu | Roots: %zu | Players: %zu",
        nodes.size(), roots.size(), playerTrees.size());
    ImGui::Separator();

    float leftWidth = ImGui::GetContentRegionAvail().x * 0.4f;

    if (ImGui::BeginChild("##Left", ImVec2(leftWidth, 0), true,
        ImGuiWindowFlags_HorizontalScrollbar))
    {
        if (ImGui::CollapsingHeader("Player Trees", ImGuiTreeNodeFlags_DefaultOpen))
            this->DrawPlayerTrees(playerTrees, nodes);

        ImGui::Spacing();

        if (ImGui::CollapsingHeader("World Roots", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGuiListClipper clipper;
            clipper.Begin(static_cast<int>(roots.size()));
            while (clipper.Step())
            {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                    this->DrawNodeHierarchy(roots[i], nodes);
            }
            clipper.End();
        }

        ImGui::EndChild();
    }

    ImGui::SameLine();

    if (ImGui::BeginChild("##Right", ImVec2(0, 0), true))
    {
        auto it = nodes.find(m_SelectedHandle);
        if (it != nodes.end())
            this->DrawSelectedNodeDetails(it->second);
        else
            ImGui::TextDisabled("Select a node to view details.");

        ImGui::EndChild();
    }
}

void UI_ObjectGraph::Cleanup()
{
    m_SelectedHandle = 0xFFFFFFFF;
    m_DiscoveryOrder.clear();
}

void UI_ObjectGraph::DrawPlayerTrees(
    const std::vector<PlayerTree>& trees,
    const std::unordered_map<uint32_t, ObjectNode>& nodes)
{
    for (const auto& tree : trees)
    {
        auto player = m_Deps.State_PlayerTable.CopyLivePlayer(tree.Handle);
        if (!player) continue;

        const char* gamertag = player->Gamertag.c_str();
        bool alive = tree.IsAlive();

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_SpanAvailWidth;

        ImVec4 nameColor = alive
            ? ImVec4(0.4f, 1.0f, 0.4f, 1.0f)
            : ImVec4(0.5f, 0.5f, 0.5f, 1.0f);

        ImGui::PushStyleColor(ImGuiCol_Text, nameColor);
        bool open = ImGui::TreeNodeEx(
            reinterpret_cast<void*>(static_cast<uintptr_t>(tree.Handle)),
            flags, "%s %s", gamertag, alive ? "" : "(dead)");
        ImGui::PopStyleColor();

        if (!open) continue;

        auto drawLeaf = [&](const char* label, uint32_t handle) {
            if (handle == 0xFFFFFFFF) return;
        
            auto obj = m_Deps.State_ObjectTable.CopyLiveObject(handle);
            const char* detail = obj ? obj->FourCC.c_str() : "?";
        
            ImGuiTreeNodeFlags leafFlags =
                ImGuiTreeNodeFlags_Leaf |
                ImGuiTreeNodeFlags_NoTreePushOnOpen |
                ImGuiTreeNodeFlags_SpanAvailWidth;
        
            if (m_SelectedHandle == handle)
                leafFlags |= ImGuiTreeNodeFlags_Selected;
        
            ImGui::TreeNodeEx(
                reinterpret_cast<void*>(static_cast<uintptr_t>(handle)),
                leafFlags, "%s [%s] 0x%08X", label, detail, handle);
        
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
                m_SelectedHandle = handle;
        };

        drawLeaf("Biped", tree.BipedHandle);
        drawLeaf("Primary Weapon", tree.PrimaryWeaponHandle);
        drawLeaf("Secondary Weapon", tree.SecondaryWeaponHandle);
        drawLeaf("Ability", tree.AbilityHandle);
        drawLeaf("Objective", tree.ObjectiveHandle);
        drawLeaf("Vehicle", tree.VehicleHandle);

        if (!tree.VehiclePartHandles.empty())
        {
            if (ImGui::TreeNodeEx("Vehicle Parts",
                ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth))
            {
                for (uint32_t partHandle : tree.VehiclePartHandles)
                    drawLeaf("Part", partHandle);
                ImGui::TreePop();
            }
        }

        ImGui::TreePop();
    }
}

void UI_ObjectGraph::DrawNodeHierarchy(
    uint32_t handle,
    const std::unordered_map<uint32_t, ObjectNode>& nodes)
{
    auto it = nodes.find(handle);
    if (it == nodes.end()) return;

    const auto& node = it->second;

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (node.ChildrenHandles.empty()) flags |= ImGuiTreeNodeFlags_Leaf;
    if (m_SelectedHandle == handle)   flags |= ImGuiTreeNodeFlags_Selected;

    auto obj = m_Deps.State_ObjectTable.CopyLiveObject(handle);
    const char* cls = obj ? obj->FourCC.c_str() : "?";

    bool open = ImGui::TreeNodeEx(
        reinterpret_cast<void*>(static_cast<uintptr_t>(handle)),
        flags, "[%s] 0x%08X", cls, handle);

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        m_SelectedHandle = handle;

    if (open)
    {
        for (uint32_t childHandle : node.ChildrenHandles)
            this->DrawNodeHierarchy(childHandle, nodes);
        ImGui::TreePop();
    }
}

void UI_ObjectGraph::DrawSelectedNodeDetails(const ObjectNode& node)
{
    auto obj = m_Deps.State_ObjectTable.CopyLiveObject(node.Handle);

    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f);
    if (ImGui::BeginChild("##NodeDetail", ImVec2(0, 0), true))
    {
        if (obj)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
            ImGui::TextWrapped("%s", obj->TagName.c_str());
            ImGui::PopStyleColor();

            ImGui::Separator();
            ImGui::TextDisabled("Class: %s", obj->FourCC.c_str());

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.4f, 1.0f), "Position");
            ImGui::Text("%.3f  %.3f  %.3f",
                obj->Position[0], obj->Position[1], obj->Position[2]);
        }

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1.0f), "Graph");
        ImGui::Separator();

        auto drawHandle = [](const char* label, uint32_t h)
            {
                if (h == 0xFFFFFFFF)
                    ImGui::TextDisabled("%s: NONE", label);
                else
                    ImGui::Text("%s: 0x%08X", label, h);
            };

        drawHandle("Handle", node.Handle);
        drawHandle("Parent", node.ParentHandle);

        if (!node.ChildrenHandles.empty())
        {
            ImGui::Spacing();
            ImGui::TextDisabled("Children (%zu):", node.ChildrenHandles.size());
            for (uint32_t ch : node.ChildrenHandles)
                ImGui::Text("  0x%08X", ch);
        }

        ImGui::EndChild();
    }
    ImGui::PopStyleVar();
}