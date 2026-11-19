module;

#include "External/imgui/imgui.h"

module DevTools.Viewer.Inspector.System;
import :Vitality;

import Blam.MapModel.Vitality.Type;
import Blam.MapModel.World.Type;
import DevTools.Gui.Format.System;
import DevTools.Gui.Widget.System;
import std;

namespace
{
    using Bridge::Codec::Tick::Type::Tick;
    using Perception::Details::Type::VitalityDetail;
    using Blam::MapModel::Vitality::Type::Vitality::Section;
    using Blam::MapModel::Vitality::Type::Vitality::VitalityKind;
    using Blam::MapModel::World::Type::ModelLink::ModelLinkAnchor;
    using Blam::MapModel::World::Type::ModelLink::AnchorSource;
    using Blam::MapModel::Vitality::Type::Vitality::Vitality;
    using DevTools::Gui::Format::System::HexFormater;

    using DevTools::Gui::Widget::System::PanelGuiService;

    const ImVec4 k_Critical{ 1.0f, 0.5f, 0.3f, 1.0f };
    const ImVec4 k_Headshot{ 1.0f, 0.85f, 0.2f, 1.0f };
    const ImVec4 k_Shield{ 0.4f, 0.7f, 1.0f, 1.0f };
    const ImVec4 k_Destroys{ 0.85f, 0.45f, 0.85f, 1.0f };

    auto SourceName(AnchorSource source) -> const char*
    {
        switch (source)
        {
        case AnchorSource::ModelTarget:    return "target";
        case AnchorSource::HeadshotTarget: return "head target";
        case AnchorSource::CollRegion:     return "region";
        case AnchorSource::ObjectCenter:   return "center";
        default:                           return "none";
        }
    }

    auto DrawRoleTags(const Section& section) -> void
    {
        if (section.IsCritical)
        {
            ImGui::SameLine();
            ImGui::TextColored(k_Critical, "[critical]");
        }

        if (section.IsHeadshot)
        {
            ImGui::SameLine();
            ImGui::TextColored(k_Headshot, "[headshot]");
        }

        if (section.DestroysObject)
        {
            ImGui::SameLine();
            ImGui::TextColored(k_Destroys, "[destroys]");
        }

        if (section.Kind == VitalityKind::Shield)
        {
            ImGui::SameLine();
            ImGui::TextColored(k_Shield, "[shield]");
        }
    }

    auto DrawAimLine(const ModelLinkAnchor& aim) -> void
    {
        if (aim.Source == AnchorSource::None)
        {
            ImGui::TextDisabled("  aim: none");
            return;
        }

        const std::string line = std::format(
            "  aim: {} | node {} | off {:.3f}, {:.3f}, {:.3f} | r {:.3f}",
            SourceName(aim.Source), aim.ModelNodeIndex,
            aim.LocalOffset.X, aim.LocalOffset.Y, aim.LocalOffset.Z, aim.Radius);

        ImGui::TextDisabled("%s", line.c_str());
    }

    // --- Vitality (static layout, Resolved) ---

    auto DrawVitalitySection(const Section& section) -> void
    {
        ImGui::Text("Section %u", static_cast<unsigned>(section.NameId));
        DrawRoleTags(section);

        if (section.CollRegion < 0 && section.Kind != VitalityKind::Shield)
        {
            ImGui::SameLine();
            ImGui::TextDisabled("(no coll)");
        }

        DrawAimLine(section.Aim);
    }

    auto DrawVitality(const std::string* tagName, const Vitality* layout) -> void
    {
        if (tagName) ImGui::TextWrapped("%s", tagName->c_str());

        if (!layout)
        {
            ImGui::TextDisabled("No vitality layout.");
            return;
        }

        ImGui::Text("Max vitality: %.1f | Max shield: %.1f",
            layout->MaximumVitality, layout->MaximumShieldVitality);

        ImGui::Text("Kill: %zu | Headshot: %zu | Shield: %zu",
            layout->KillSections.size(), layout->HeadshotSections.size(),
            layout->ShieldSections.size());

        ImGui::TextDisabled("Object center: %s", SourceName(layout->ObjectCenter.Source));

        ImGui::Spacing();
        ImGui::TextColored({ 0.8f, 0.8f, 0.8f, 1.0f }, "Sections (%d)",
            static_cast<int>(layout->Sections.size()));
        ImGui::Spacing();

        for (const Section& section : layout->Sections)
        {
            DrawVitalitySection(section);
            ImGui::Spacing();
        }
    }

    // --- Health (dynamic, per tick) ---

    auto DrawHealthRow(std::size_t index, float vitality, const Section* section) -> void
    {
        const bool isShield = section && section->Kind == VitalityKind::Shield;
        const bool isCritical = section && section->IsCritical;

        if (section) ImGui::Text("Section %u", static_cast<unsigned>(section->NameId));
        else         ImGui::Text("Section #%zu", index);

        if (section) DrawRoleTags(*section);

        const float clamped = std::clamp(vitality, 0.0f, 1.0f);

        const ImVec4 barColor = isShield
            ? ImVec4(0.35f, 0.65f, 1.0f, 1.0f)
            : (isCritical
                ? ImVec4(0.9f, 0.45f, 0.3f, 1.0f)
                : ImVec4(0.45f, 0.85f, 0.5f, 1.0f));

        const std::string overlay = std::format("{:.2f}", vitality);

        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, barColor);
        ImGui::ProgressBar(clamped, ImVec2(-1.0f, 0.0f), overlay.c_str());
        ImGui::PopStyleColor();

        ImGui::Spacing();
    }

    auto DrawHealth(const VitalityDetail& detail, const Vitality* layout) -> void
    {
        ImGui::TextColored(ImVec4(0.4f, 0.86f, 1.0f, 1.0f), "Health");
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Handle: %s", HexFormater::Hex32(detail.Handle).c_str());

        // note: The lifecycle is in the Entity block above.
        PanelGuiService::DrawSectionSeparator(nullptr);

        ImGui::TextColored({ 0.8f, 0.8f, 0.8f, 1.0f }, "Damage Sections (%d)",
            static_cast<int>(detail.Sections.size()));
        ImGui::Spacing();

        for (std::size_t i = 0; i < detail.Sections.size(); ++i)
        {
            const Section* section = (layout && i < layout->Sections.size())
                ? &layout->Sections[i] : nullptr;

            DrawHealthRow(i, detail.Sections[i], section);
        }
    }
}

namespace DevTools::Viewer::Inspector::System
{
    auto VitalitySection::Find(const Tick& tick, std::uint32_t handle) -> const VitalityDetail*
    {
        if (!tick.VitalityDetails) return nullptr;

        const auto it{ tick.VitalityDetails->find(handle) };
        return it == tick.VitalityDetails->end() ? nullptr : &it->second;
    }

    auto VitalitySection::Draw(const Tick& tick, std::uint32_t handle, const VitalityDetail& detail) -> void
    {
        const std::string* tagName{ nullptr };

        if (tick.ObjectTable)
        {
            const auto it{ tick.ObjectTable->find(handle) };
            if (it != tick.ObjectTable->end()) tagName = &it->second.TagName;
        }

        const Vitality* layout{ detail.Layout.get() };

        DrawVitality(tagName, layout);

        ImGui::Spacing();
        ImGui::Spacing();

        DrawHealth(detail, layout);
    }
}