#include "pch.h"

#include "OverlayPanel_Vitality.h"

#include "Core/Types/Tick/Tick.h"
#include "Core/Types/Environment/Vitality/ObjectVitality.h"
#include "Core/Types/Sources/Tables/Object/LiveObject.h"

#include "Core/UI/Utils/Hex/HexFormater.h"

#include "External/imgui/imgui.h"

#include <algorithm>
#include <cstdio>

namespace
{
    void DrawSectionRow(const VitalitySection& section)
    {
        const bool isShield = (section.Kind == VitalitySectionKind::Shield);

        ImGui::Text("Section %u", static_cast<unsigned>(section.NameId));

        if (section.IsCritical)
        {
            ImGui::SameLine();
            ImGui::TextColored({ 1.0f, 0.5f, 0.3f, 1.0f }, "[critical]");
        }
        if (section.IsHeadshot)
        {
            ImGui::SameLine();
            ImGui::TextColored({ 1.0f, 0.85f, 0.2f, 1.0f }, "[headshot]");
        }
        if (isShield)
        {
            ImGui::SameLine();
            ImGui::TextColored({ 0.4f, 0.7f, 1.0f, 1.0f }, "[shield]");
        }
        if (section.CollRegion < 0 && !isShield)
        {
            ImGui::SameLine();
            ImGui::TextDisabled("(no coll)");
        }

        const float v = (std::max)(0.0f, (std::min)(section.Vitality, 1.0f));

        ImVec4 barColor = isShield
            ? ImVec4(0.35f, 0.65f, 1.0f, 1.0f)
            : (section.IsCritical
                ? ImVec4(0.9f, 0.45f, 0.3f, 1.0f)
                : ImVec4(0.45f, 0.85f, 0.5f, 1.0f));

        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, barColor);
        char overlay[32];
        snprintf(overlay, sizeof(overlay), "%.2f", section.Vitality);
        ImGui::ProgressBar(v, ImVec2(-1.0f, 0.0f), overlay);
        ImGui::PopStyleColor();

        ImGui::Spacing();
    }

    void DrawObjectVitality(const ObjectVitality& vitality,
        const ObjectTable* objects)
    {
        const LiveObject* obj = nullptr;
        if (objects)
        {
            auto it = objects->find(vitality.Handle);
            if (it != objects->end()) obj = &it->second;
        }

        if (obj)
            ImGui::TextWrapped("%s", obj->TagName.c_str());

        ImGui::Text("Handle: %s", HexFormater::Hex32(vitality.Handle).c_str());

        ImGui::Spacing();
        if (vitality.IsDead)
            ImGui::TextColored({ 1.0f, 0.35f, 0.35f, 1.0f }, "[ DEAD ]");
        else
            ImGui::TextColored({ 0.4f, 1.0f, 0.4f, 1.0f }, "[ ALIVE ]");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextColored({ 0.8f, 0.8f, 0.8f, 1.0f }, "Damage Sections (%d)",
            static_cast<int>(vitality.Sections.size()));
        ImGui::Spacing();

        for (const VitalitySection& section : vitality.Sections)
            DrawSectionRow(section);
    }
}

void OverlayPanel_Vitality::Draw(
    const std::shared_ptr<const Tick>& tick, uint32_t handle)
{
    ImGui::TextColored(ImVec4(0.4f, 0.86f, 1.0f, 1.0f), "Vitality");
    ImGui::Separator();
    ImGui::Spacing();

    if (!tick || !tick->Vitalities)
    {
        ImGui::TextDisabled("No vitality data.");
        return;
    }

    auto it = tick->Vitalities->find(handle);
    if (it == tick->Vitalities->end())
    {
        ImGui::TextDisabled("This object has no vitality data.");
        return;
    }

    DrawObjectVitality(it->second, tick->ObjectTable.get());
}