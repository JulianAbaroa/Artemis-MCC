#include "pch.h"
#include "Core/UI/Tabs/Domain/Graph/ObjectGraphTab.h"
#include "Core/States/CoreState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Domain/Graph/ObjectGraphState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include "External/imgui/imgui.h"
#include <Algorithm>
#include <format>

void ObjectGraphTab::Draw()
{
    const auto& nodes = g_pState->Domain->ObjectGraph->GetNodes();
    const auto& playerTrees = g_pState->Domain->ObjectGraph->GetPlayerTrees();

    auto it_remove = std::remove_if(m_DiscoveryOrder.begin(), m_DiscoveryOrder.end(),
        [&](uint32_t h) { return nodes.find(h) == nodes.end(); });
    m_DiscoveryOrder.erase(it_remove, m_DiscoveryOrder.end());

    for (const auto& [handle, node] : nodes)
    {
        if (std::find(m_DiscoveryOrder.begin(), m_DiscoveryOrder.end(), handle) == m_DiscoveryOrder.end())
        {
            m_DiscoveryOrder.push_back(handle);
        }
    }

    std::vector<uint32_t> currentRoots;
    currentRoots.reserve(m_DiscoveryOrder.size());
    for (uint32_t handle : m_DiscoveryOrder)
    {
        auto it = nodes.find(handle);
        if (it != nodes.end() && it->second.ParentHandle == 0xFFFFFFFF)
        {
            currentRoots.push_back(handle);
        }
    }

    ImGui::TextDisabled("Total Nodes: %zu | Roots: %zu | Players: %zu",
        nodes.size(), currentRoots.size(), playerTrees.size());
    ImGui::Separator();

    float leftPanelWidth = ImGui::GetContentRegionAvail().x * 0.4f;

    if (ImGui::BeginChild("##GraphTreeRegion", ImVec2(leftPanelWidth, 0), true, ImGuiWindowFlags_HorizontalScrollbar))
    {
        if (ImGui::CollapsingHeader("Player Trees", ImGuiTreeNodeFlags_DefaultOpen))
        {
            this->DrawPlayerTrees(playerTrees, nodes);
        }

        ImGui::Spacing();

        if (ImGui::CollapsingHeader("World Object Roots (Stable)", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGuiListClipper clipper;
            clipper.Begin(static_cast<int>(currentRoots.size()));

            while (clipper.Step())
            {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                {
                    uint32_t handle = currentRoots[i];
                    this->DrawNodeHierarchy(handle, nodes);
                }
            }
            clipper.End();
        }
        ImGui::EndChild();
    }

    ImGui::SameLine();

    if (ImGui::BeginChild("##NodeDetailsRegion", ImVec2(0, 0), true))
    {
        auto it = nodes.find(m_SelectedHandle);
        if (it != nodes.end())
            this->DrawSelectedNodeDetails(it->second);
        else
            ImGui::TextDisabled("Select a node to view details.");

        ImGui::EndChild();
    }
}


void ObjectGraphTab::Cleanup()
{
    m_SelectedHandle = 0xFFFFFFFF;
    m_DiscoveryOrder.clear();
}


void ObjectGraphTab::DrawPlayerTrees(const std::vector<PlayerObjectTree>& trees,
    const std::unordered_map<uint32_t, ObjectNode>& nodes)
{
    for (const auto& tree : trees)
    {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

        bool isOpen = ImGui::TreeNodeEx(
            reinterpret_cast<void*>(static_cast<uintptr_t>(tree.PlayerHandle)),
            flags, "Player: %s", tree.Gamertag.c_str());

        if (isOpen)
        {
            auto drawPlayerLeaf = [&](const char* label, uint32_t handle) {
                if (handle == 0xFFFFFFFF) return;

                auto it = nodes.find(handle);
                if (it == nodes.end()) return;

                const auto& node = it->second;
                ImGuiTreeNodeFlags leafFlags = ImGuiTreeNodeFlags_Leaf |
                    ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;

                if (m_SelectedHandle == handle) leafFlags |= ImGuiTreeNodeFlags_Selected;

                ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<uintptr_t>(handle)),
                    leafFlags, "%s: %s", label, node.Class.c_str());

                if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
                    m_SelectedHandle = handle;
                };

            drawPlayerLeaf("Biped", tree.BipedHandle);
            drawPlayerLeaf("Primary Weapon", tree.PrimaryWeaponHandle);
            drawPlayerLeaf("Secondary Weapon", tree.SecondaryWeaponHandle);
            drawPlayerLeaf("Vehicle", tree.VehicleHandle);
            drawPlayerLeaf("Objective", tree.ObjectiveHandle);

            if (!tree.VehicleSiblingHandles.empty())
            {
                if (ImGui::TreeNodeEx("Vehicle Parts", ImGuiTreeNodeFlags_OpenOnArrow))
                {
                    for (uint32_t sibHandle : tree.VehicleSiblingHandles)
                        drawPlayerLeaf("Part", sibHandle);
                    ImGui::TreePop();
                }
            }
            ImGui::TreePop();
        }
    }
}

void ObjectGraphTab::DrawNodeHierarchy(uint32_t handle, const std::unordered_map<uint32_t, ObjectNode>& nodes)
{
    auto it = nodes.find(handle);
    if (it == nodes.end()) return;

    const auto& node = it->second;

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (node.ChildHandle == 0xFFFFFFFF) flags |= ImGuiTreeNodeFlags_Leaf;
    if (m_SelectedHandle == handle) flags |= ImGuiTreeNodeFlags_Selected;

    bool isOpen = ImGui::TreeNodeEx((void*)(uintptr_t)handle, flags,
        "[%s] 0x%08X", node.Class.c_str(), handle);

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        m_SelectedHandle = handle;

    if (isOpen)
    {
        uint32_t currentChildHandle = node.ChildHandle;

        while (currentChildHandle != 0xFFFFFFFF)
        {
            auto childIt = nodes.find(currentChildHandle);
            if (childIt == nodes.end()) break;

            this->DrawNodeHierarchy(currentChildHandle, nodes);

            currentChildHandle = childIt->second.NextSiblingHandle;
        }

        ImGui::TreePop();
    }
}

void ObjectGraphTab::DrawSelectedNodeDetails(const ObjectNode& node)
{
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f);

    if (ImGui::BeginChild("NodeCardDetail", ImVec2(0, 0), true))
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
        ImGui::Text("Tag: %s", node.TagName.c_str());
        ImGui::PopStyleColor();

        ImGui::Separator();
        ImGui::TextDisabled("Class: %s", node.Class.c_str());
        ImGui::Text("Handle: 0x%08X", node.Handle);

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1.0f), "Relationships (Graph Topology)");
        ImGui::Separator();

        auto drawHandleInfo = [](const char* label, uint32_t h) {
            ImGui::Text("%s: %s", label, h == 0xFFFFFFFF ? "NONE" : std::format("0x{:08X}", h).c_str());
            };

        drawHandleInfo("Parent Handle", node.ParentHandle);
        drawHandleInfo("Child Handle", node.ChildHandle);
        drawHandleInfo("Sibling Handle", node.NextSiblingHandle);
    }

    ImGui::EndChild();
    ImGui::PopStyleVar();
}