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
    using RegionDiagnostic = Export::Tick::Type::RegionDiagnostic;

    using PanelGuiService = Gui::Widget::System::PanelGuiService;

    constexpr ImVec4 k_AlertColor{ 1.0f, 0.4f, 0.4f, 1.0f };
    constexpr std::size_t k_BytesPerRow{ 16 };

    // Formats a permutation or state index, or a dash when it is missing.
    auto FormatIndex(int value) -> std::string
    {
        return value < 0 ? std::string{ "-" } : std::to_string(value);
    }

    // Formats the distinct permutations with how many parts show each one.
    auto FormatPermutations(const std::vector<int>& permutations) -> std::string
    {
        if (permutations.empty()) return "-";

        std::map<int, int> counts{};

        for (int permutation : permutations)
        {
            ++counts[permutation];
        }

        std::string text{};

        for (const auto& [permutation, count] : counts)
        {
            if (!text.empty()) text += ",";

            text += std::to_string(permutation);

            if (count > 1) text += std::format("x{}", count);
        }

        return text;
    }

    // Tells if the viewer shows only the permutation the engine has, or none when the engine hides the region.
    auto IsShownAsEngine(const RegionDiagnostic& region) -> bool
    {
        if (region.EnginePermutation < 0) return region.ShownPermutations.empty();

        // The collision model lacks that permutation: the region keeps its default while the render model draws it.
        if (region.MappedPermutation < 0)
        {
            return region.EngineMeshCount > 0 ? !region.ShownPermutations.empty() : region.ShownPermutations.empty();
        }

        return !region.ShownPermutations.empty() &&
            std::ranges::all_of(region.ShownPermutations, [&](int permutation) {
                return permutation == region.MappedPermutation;
            });
    }

    // Formats the bytes of the region block as hex rows with their offset.
    auto FormatBlockRows(const std::vector<std::uint8_t>& bytes) -> std::vector<std::string>
    {
        std::vector<std::string> rows{};

        for (std::size_t row = 0; row < bytes.size(); row += k_BytesPerRow)
        {
            std::string line = std::format("+{:02X}:", row);

            const std::size_t end = (std::min)(row + k_BytesPerRow, bytes.size());

            for (std::size_t current = row; current < end; ++current)
            {
                line += std::format(" {:02X}", bytes[current]);
            }

            rows.push_back(std::move(line));
        }

        return rows;
    }

    auto DrawCell(const std::string& text, bool isAlert) -> void
    {
        if (isAlert)
        {
            ImGui::TextColored(k_AlertColor, "%s", text.c_str());
            return;
        }

        ImGui::TextUnformatted(text.c_str());
    }

    // Draws the damage state of each region as the engine keeps it, against what the viewer shows.
    auto DrawRegions(const Collidable& instance) -> void
    {
        PanelGuiService::DrawSectionSeparator();

        ImGui::TextDisabled("Region states (engine). F7 prints them to the logs:");

        if (instance.RegionBlock.empty())
        {
            ImGui::TextDisabled("No region block.");
            return;
        }

        ImGui::Text("Block: size 0x%X, offset 0x%X", instance.RegionBlockSize, instance.RegionBlockOffset);
        ImGui::Text("Engine regions: %d", static_cast<int>(instance.Regions.size()));

        if (ImGui::BeginTable("##regionstates", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn("#");
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Coll");
            ImGui::TableSetupColumn("State");
            ImGui::TableSetupColumn("Engine");
            ImGui::TableSetupColumn("Mapped");
            ImGui::TableSetupColumn("Meshes");
            ImGui::TableSetupColumn("Viewer");
            ImGui::TableHeadersRow();

            for (std::size_t current = 0; current < instance.Regions.size(); ++current)
            {
                const RegionDiagnostic& region = instance.Regions[current];

                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::Text("%d", static_cast<int>(current));

                ImGui::TableNextColumn();
                ImGui::Text("0x%X", region.Name);

                ImGui::TableNextColumn();
                ImGui::TextUnformatted(FormatIndex(region.CollRegion).c_str());

                ImGui::TableNextColumn();
                ImGui::TextUnformatted(FormatIndex(region.EngineState).c_str());

                ImGui::TableNextColumn();
                ImGui::TextUnformatted(FormatIndex(region.EnginePermutation).c_str());

                ImGui::TableNextColumn();
                ImGui::TextUnformatted(FormatIndex(region.MappedPermutation).c_str());

                ImGui::TableNextColumn();
                ImGui::TextUnformatted(FormatIndex(region.EngineMeshCount).c_str());

                ImGui::TableNextColumn();
                DrawCell(FormatPermutations(region.ShownPermutations),
                    !IsShownAsEngine(region));
            }

            ImGui::EndTable();
        }

        ImGui::TextDisabled("Raw block:");

        for (const std::string& row : FormatBlockRows(instance.RegionBlock))
        {
            ImGui::TextUnformatted(row.c_str());
        }
    }

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

        DrawRegions(instance);
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