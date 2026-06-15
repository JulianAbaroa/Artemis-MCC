#include "pch.h"

#include "OverlayPanel_Affordance.h"

#include "Core/Types/Tick/Tick.h"
#include "Core/Types/Egocentric/Affordance/Affordance.h"
#include "Core/Types/Sources/Tables/Object/LiveObject.h"
#include "Core/Types/Sources/Tables/Interaction/LiveInteraction.h"

#include "Core/UI/Utils/String/EnumToString.h"
#include "Core/UI/Utils/Color/RoleToColor.h"

#include "External/imgui/imgui.h"

#include <string>

namespace
{
    const LiveObject* FindObject(const ObjectTable* objects, uint32_t handle)
    {
        if (!objects) return nullptr;
        auto it = objects->find(handle);
        return (it != objects->end()) ? &it->second : nullptr;
    }

    std::string ShortTag(const ObjectTable* objects, uint32_t handle)
    {
        const LiveObject* o = FindObject(objects, handle);
        if (!o) return std::string();
        const std::string& tag = o->TagName;
        size_t slash = tag.find_last_of("\\/");
        return (slash == std::string::npos) ? tag : tag.substr(slash + 1);
    }

    void DrawEngineInteraction(const LiveInteraction& interaction)
    {
        ImGui::TextColored({ 0.8f, 0.8f, 0.8f, 1.f }, "Engine Interaction State");
        ImGui::Spacing();

        bool hasObject = interaction.TargetObjectHandle != 0xFFFFFFFF
            && interaction.TargetObjectHandle != 0;

        if (hasObject)
        {
            ImGui::TextColored({ 0.2f, 1.f, 0.2f, 1.f }, "Object");
            ImGui::Text("  Type:   %s",
                EnumToString::InteractionTypeToString(interaction.Type));
            ImGui::Text("  Handle: 0x%08X", interaction.TargetObjectHandle);
            ImGui::Text("  Detail: %s", EnumToString::InteractionDetailToString(
                interaction.Type, interaction.InteractionSlotID));
        }
        else
        {
            ImGui::TextDisabled("Object - none");
        }

        ImGui::Spacing();

        bool hasMelee = interaction.IsMeleeAvailable == 0x0E
            && interaction.MeleeTargetHandle != 0xFFFFFFFF;
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

        bool hasAim = interaction.IsAimAvailable == 0x01
            && interaction.AimTargetHandle != 0xFFFFFFFF;
        if (hasAim)
        {
            ImGui::TextColored({ 0.4f, 0.8f, 1.f, 1.f }, "Aim");
            ImGui::Text("  Target:   0x%08X", interaction.AimTargetHandle);
            ImGui::Text("  SlotID:   0x%08X", interaction.AimTargetSlotID);
            ImGui::Text("  ModelPart: 0x%02X", interaction.ModelPart);
            ImGui::Text("  LocalPos: %.3f %.3f %.3f",
                interaction.AimHitLocalPosition[0],
                interaction.AimHitLocalPosition[1],
                interaction.AimHitLocalPosition[2]);
        }
        else
        {
            ImGui::TextDisabled("Aim - none");
        }
    }

    void DrawDetails(const Affordance& item, const ObjectTable* objects)
    {
        const LiveObject* objPtr = FindObject(objects, item.Handle);

        // --- Identity ---
        ImGui::PushStyleColor(ImGuiCol_Text, RoleToColor::FromRole(item.Role));
        if (objPtr) ImGui::TextWrapped("%s", objPtr->TagName.c_str());
        else        ImGui::TextWrapped("0x%08X", item.Handle);
        ImGui::PopStyleColor();

        ImGui::Text("Handle: 0x%08X", item.Handle);
        ImGui::Text("Role:   %s", EnumToString::RoleToString(item.Role));

        if (objPtr && objPtr->ParentHandle != 0xFFFFFFFF)
            ImGui::Text("Parent: 0x%08X", objPtr->ParentHandle);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // --- Interaction state ---
        if (item.IsEngineSelected)
            ImGui::TextColored({ 0.2f, 1.f, 0.2f, 1.f }, "[ ACTION READY - ENGINE SELECTED ]");
        else if (item.Activation == AffordanceActivation::None)
            ImGui::TextColored({ 0.6f, 0.6f, 0.6f, 1.f }, "[ WORLD ENTITY - NO DIRECT INTERACTION ]");
        else
            ImGui::TextColored({ 1.f, 0.4f, 0.4f, 1.f }, "[ OUT OF RANGE ]");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // --- Classification ---
        ImGui::TextColored({ 0.8f, 0.8f, 0.8f, 1.f }, "Classification");
        ImGui::Text("Activation: %s", EnumToString::ActivationToString(item.Activation));
        ImGui::Text("Behaviors:  ");
        for (size_t i = 0; i < item.Behaviors.size(); ++i)
        {
            ImGui::SameLine();
            if (i > 0) { ImGui::TextDisabled("|"); ImGui::SameLine(); }
            ImGui::TextDisabled("%s", EnumToString::BehaviorToString(item.Behaviors[i]));
        }

        // --- Spatial data ---
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextColored({ 0.8f, 0.8f, 0.8f, 1.f }, "Spatial Data");
        ImGui::Text("Distance: %.2f m", item.DistanceToPlayer);
        ImGui::Text("Position: %.3f  %.3f  %.3f",
            item.Position[0], item.Position[1], item.Position[2]);

        if (objPtr)
        {
            ImGui::Text("Forward:  %.3f  %.3f  %.3f",
                objPtr->Forward[0], objPtr->Forward[1], objPtr->Forward[2]);
            ImGui::Text("Up:       %.3f  %.3f  %.3f",
                objPtr->Up[0], objPtr->Up[1], objPtr->Up[2]);
            ImGui::Text("LinVel:   %.3f  %.3f  %.3f",
                objPtr->LinearVelocity[0], objPtr->LinearVelocity[1], objPtr->LinearVelocity[2]);
            ImGui::Text("AngVel:   %.3f  %.3f  %.3f",
                objPtr->AngularVelocity[0], objPtr->AngularVelocity[1], objPtr->AngularVelocity[2]);
        }

        // --- Object Profile ---
        if (objPtr)
        {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextColored({ 0.8f, 0.8f, 0.8f, 1.f }, "Object Profile");

            const ObjectProfile& p = objPtr->Profile;

            auto BoolBadge = [](const char* label, bool value) {
                ImGui::Text("%-10s", label);
                ImGui::SameLine();
                if (value) ImGui::TextColored({ 0.4f, 1.f, 0.4f, 1.f }, "yes");
                else       ImGui::TextColored({ 0.4f, 0.4f, 0.4f, 1.f }, "no");
                };

            if (ImGui::BeginTable("##profile", 2))
            {
                ImGui::TableNextColumn(); BoolBadge("HasPhmo:", p.HasPhmo);
                ImGui::TableNextColumn(); BoolBadge("HasColl:", p.HasColl);
                ImGui::TableNextColumn(); BoolBadge("HasMode:", p.HasMode);
                ImGui::TableNextColumn(); BoolBadge("HasWeap:", p.HasWeap);
                ImGui::TableNextColumn(); BoolBadge("HasVehi:", p.HasVehi);
                ImGui::TableNextColumn(); BoolBadge("HasEqip:", p.HasEqip);
                ImGui::TableNextColumn(); BoolBadge("HasBloc:", p.HasBloc);
                ImGui::TableNextColumn(); BoolBadge("HasCtrl:", p.HasCtrl);
                ImGui::TableNextColumn(); BoolBadge("HasProj:", p.HasProj);
                ImGui::EndTable();
            }
        }

        // --- Vehicle Seats ---
        if (!item.Seats.empty())
        {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextColored({ 1.f, 0.8f, 0.2f, 1.f },
                "Vehicle Seats (%zu)", item.Seats.size());

            if (ImGui::BeginTable("##seats", 5,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
            {
                ImGui::TableSetupColumn("Seat");
                ImGui::TableSetupColumn("Type");
                ImGui::TableSetupColumn("Status");
                ImGui::TableSetupColumn("Occupant");
                ImGui::TableSetupColumn("Dist");
                ImGui::TableHeadersRow();

                for (const SeatStatus& seat : item.Seats)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", seat.SeatName.c_str());
                    ImGui::TableSetColumnIndex(1);
                    if (seat.IsHijackerSlot)
                        ImGui::TextColored({ 1.f, 0.6f, 0.1f, 1.f }, "Hijack");
                    else
                        ImGui::TextDisabled("Normal");
                    ImGui::TableSetColumnIndex(2);
                    if (seat.IsOccupied)
                        ImGui::TextColored({ 1.f, 0.4f, 0.4f, 1.f }, "Occupied");
                    else
                        ImGui::TextColored({ 0.4f, 1.f, 0.4f, 1.f }, "Free");
                    ImGui::TableSetColumnIndex(3);
                    if (seat.OccupyingBipedHandle == 0xFFFFFFFF)
                        ImGui::TextDisabled("-");
                    else
                        ImGui::Text("0x%08X", seat.OccupyingBipedHandle);
                    ImGui::TableSetColumnIndex(4);
                    ImGui::Text("%.2f m", seat.DistanceToPlayer);
                }
                ImGui::EndTable();
            }
        }

        // --- Child Vehicles ---
        if (!item.ChildHandles.empty())
        {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextColored({ 1.f, 0.8f, 0.2f, 1.f },
                "Child Vehicles (%zu)", item.ChildHandles.size());

            for (uint32_t ch : item.ChildHandles)
                ImGui::Text("  0x%08X  %s", ch, ShortTag(objects, ch).c_str());
        }
    }
}

void OverlayPanel_Affordance::Draw(
    const std::shared_ptr<const Tick>& tick, uint32_t handle)
{
    if (!tick)
    {
        ImGui::TextDisabled("No data.");
        return;
    }

    const LiveInteraction interaction = tick->Interaction
        ? *tick->Interaction : LiveInteraction{};
    DrawEngineInteraction(interaction);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (!tick->Affordances)
    {
        ImGui::TextDisabled("No affordance data.");
        return;
    }

    for (const auto& item : *tick->Affordances)
    {
        if (item.Handle == handle)
        {
            DrawDetails(item, tick->ObjectTable.get());
            return;
        }
    }

    ImGui::TextDisabled("Selected object is not an affordance.");
}