module;

#include "External/imgui/imgui.h"

module DevTools.Viewer.Inspector.System;
import :Raw;

import Blam.Runtime.Interaction.Type;
import DevTools.Gui.Style.Type;
import DevTools.Gui.Widget.System;
import DevTools.Viewer.Format.System;
import std;

namespace
{
    using Blam::Runtime::Interaction::Type::Alive::AliveInteraction;
    using Perception::Collidable::Type::Collidable;
    using Perception::Collidable::Type::RegionDiagnostic;

    using DevTools::Viewer::Format::System::InteractionFormater;

    using DevTools::Gui::Widget::System::PanelGuiService;

    using DevTools::Gui::Style::Type::k_BytesPerRow;
    using DevTools::Gui::Style::Type::k_SectionColor;

    constexpr ImVec4 k_AlertColor{ 1.0f, 0.4f, 0.4f, 1.0f };

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

        ImGui::TextDisabled("Region states (engine):");

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

    auto DrawEngineInteraction(const AliveInteraction& interaction) -> void
    {
        ImGui::TextColored(k_SectionColor, "Engine Interaction State");
        ImGui::Spacing();

        const bool hasObject = interaction.HasObjectTarget();

        if (hasObject)
        {
            ImGui::TextColored({ 0.2f, 1.f, 0.2f, 1.f }, "Object");
            ImGui::Text("  Type:   %s",
                InteractionFormater::InteractionKindToString(interaction.Kind));
            ImGui::Text("  Handle: 0x%08X", interaction.TargetObjectHandle);
            ImGui::Text("  Detail: %s", InteractionFormater::InteractionDetailToString(
                interaction.Kind, interaction.InteractionSlotID));
        }
        else
        {
            ImGui::TextDisabled("Object - none");
        }

        ImGui::Spacing();

        const bool hasMelee = interaction.HasMeleeTarget();

        if (hasMelee)
        {
            ImGui::TextColored({ 1.f, 0.4f, 0.2f, 1.f }, "Melee");
            ImGui::Text("  Target: 0x%08X", interaction.MeleeTargetHandle);
        }
        else
        {
            ImGui::TextDisabled("Melee - none");
        }

        ImGui::Spacing();

        const bool hasAim = interaction.HasAimTarget();

        if (hasAim)
        {
            ImGui::TextColored({ 0.4f, 0.8f, 1.f, 1.f }, "Aim");
            ImGui::Text("  Target:    0x%08X", interaction.AimTargetHandle);
            ImGui::Text("  SlotID:    0x%08X", interaction.AimTargetSlotID);
            ImGui::Text("  ModelPart: 0x%02X", interaction.ModelPart);
            PanelGuiService::DrawVec3("  LocalPos: ", interaction.AimHitLocalPosition);
        }
        else
        {
            ImGui::TextDisabled("Aim - none");
        }
    }
}

namespace DevTools::Viewer::Inspector::System
{
    auto RawSection::Draw(const Tick& tick, const Collidable* collidable, bool isLocalBiped) -> void
    {
        if (collidable) DrawRegions(*collidable);

        if (!isLocalBiped) return;

        if (collidable) PanelGuiService::DrawSectionSeparator();

        DrawEngineInteraction(tick.Interaction ? *tick.Interaction : AliveInteraction{});
    }
}