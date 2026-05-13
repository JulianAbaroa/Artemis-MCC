#include "pch.h"

// Header.
#include "UI_Interactable.h"

// States.
#include "Core/States/Core_State.h"
#include "Core/States/Domain/Core_State_Domain.h"

// Object.
#include "Core/States/Domain/Object/State_ObjectTable.h"

// Interaction.
#include "Core/States/Domain/Interaction/State_InteractionTable.h"

// Interactable.
#include "Core/States/Domain/Interactable/State_Interactable.h"

// ImGui.
#include "External/imgui/imgui.h"

#include <Algorithm>
#include <format>

// TODO: Move to somewhere else.
namespace
{
    const char* RoleToString(ObjectRole role)
    {
        switch (role)
        {
        case ObjectRole::HealthStation:         return "HealthStation";
        case ObjectRole::Shield:                return "Shield";
        case ObjectRole::Lift:                  return "Lift";
        case ObjectRole::Explosive:             return "Explosive";
        case ObjectRole::Teleporter:            return "Teleporter";
        case ObjectRole::DeviceMachine:         return "DeviceMachine";
        case ObjectRole::ObjectiveSpawn:        return "Objective Spawn";
        case ObjectRole::ObjectivePickup:       return "Objective Pickup";
        case ObjectRole::ObjectiveEquiped:      return "Objective Equiped";
        case ObjectRole::Spawn:                 return "Spawn";
        case ObjectRole::Projectile:            return "Projectile";
        case ObjectRole::AmmoPickup:            return "AmmoPickup";
        case ObjectRole::ArmorAbilityEquiped:   return "Armor Ability Equiped";
        case ObjectRole::ArmorAbilityPickup:    return "Armor Ability Pickup";
        case ObjectRole::WeaponEquiped:         return "WeaponEquipped";
        case ObjectRole::WeaponPickup:          return "WeaponPickup";
        case ObjectRole::VehiclePart:           return "VehiclePart";
        case ObjectRole::Vehicle:               return "Vehicle";
        case ObjectRole::Biped:                 return "Biped";
        case ObjectRole::SelfBiped:             return "SelfBiped";
        default:                                return "Unknown";
        }
    }

    const char* BehaviorToString(InteractableBehavior behavior)
    {
        switch (behavior)
        {
        case InteractableBehavior::Pickup:       return "Pickup";
        case InteractableBehavior::EnterVehicle: return "EnterVehicle";
        case InteractableBehavior::Avoid:        return "Avoid";
        case InteractableBehavior::Interact:     return "Interact";
        default:                                 return "Unknown";
        }
    }

    const char* ActivationToString(InteractableActivation act)
    {
        switch (act)
        {
        case InteractableActivation::None:      return "None";
        case InteractableActivation::KeyPress:  return "KeyPress";
        case InteractableActivation::Proximity: return "Proximity";
        default:                                return "Unknown";
        }
    }

    const char* InteractionTypeToString(InteractionType type)
    {
        switch (type)
        {
        case InteractionType::None:              return "None";
        case InteractionType::GrabWeapon:        return "GrabWeapon";
        case InteractionType::GrabArmorAbility:  return "GrabArmorAbility";
        case InteractionType::TakeHealthStation: return "TakeHealthStation";
        case InteractionType::EnterVehicle:      return "EnterVehicle";
        case InteractionType::Hijack:            return "Hijack";
        case InteractionType::GrabObjective:     return "GrabObjective";
        default:                                 return "Unknown";
        }
    }

    const char* BodyPartToString(BodyPart part)
    {
        switch (part)
        {
        case BodyPart::Chest:    return "Chest";
        case BodyPart::Head:     return "Head";
        case BodyPart::LeftLeg:  return "LeftLeg";
        case BodyPart::RightLeg: return "RightLeg";
        default:                 return "Unknown";
        }
    }

    ImVec4 GetRoleColor(ObjectRole role)
    {
        switch (role)
        {
        case ObjectRole::Vehicle:
        case ObjectRole::VehiclePart:
            return { 1.0f, 0.8f, 0.2f, 1.0f };  // yellow

        case ObjectRole::WeaponPickup:
        case ObjectRole::WeaponEquiped:
            return { 0.4f, 0.8f, 1.0f, 1.0f };  // sky blue

        case ObjectRole::ArmorAbilityPickup:
        case ObjectRole::ArmorAbilityEquiped:
            return { 0.6f, 1.0f, 0.6f, 1.0f };  // light green

        case ObjectRole::HealthStation:
            return { 1.0f, 0.5f, 0.8f, 1.0f };  // pink

        case ObjectRole::AmmoPickup:
            return { 0.8f, 0.6f, 0.3f, 1.0f };  // orange

        case ObjectRole::Projectile:
            return { 1.0f, 0.3f, 0.3f, 1.0f };  // red

        case ObjectRole::Biped:
        case ObjectRole::SelfBiped:
            return { 0.8f, 0.6f, 1.0f, 1.0f };  // purple

        case ObjectRole::DeviceMachine:
        case ObjectRole::Teleporter:
        case ObjectRole::Lift:
        case ObjectRole::Shield:
            return { 0.5f, 0.9f, 0.9f, 1.0f };  // teal

        case ObjectRole::Explosive:
            return { 1.0f, 0.4f, 0.1f, 1.0f };  // deep orange

        default:
            return { 0.7f, 0.7f, 0.7f, 1.0f };  // gray
        }
    }

    ImVec4 GetItemColor(const AIInteractable& item)
    {
        if (item.IsEngineSelected) return { 0.2f, 1.0f, 0.2f, 1.0f };
        return GetRoleColor(item.Role);
    }
}

void UI_Interactable::Draw()
{
    std::vector<AIInteractable> interactables =
        g_pState->Domain->Interactable->GetInteractables();

    std::sort(interactables.begin(), interactables.end(),
        [](const AIInteractable& a, const AIInteractable& b) {
            return a.DistanceToPlayer < b.DistanceToPlayer;
        });

    const LiveInteraction interaction =
        g_pState->Domain->InteractionTable->GetLiveInteraction();

    // Header bar.
    ImGui::TextDisabled("Interactables: %zu", interactables.size());
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    if (interaction.IsMeleeAvailable == 0x0E)
        ImGui::TextColored({ 1.f, 0.4f, 0.2f, 1.f }, "MELEE");
    else
        ImGui::TextDisabled("melee");

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    if (interaction.IsAimAvailable == 0x01)
        ImGui::TextColored({ 0.4f, 0.8f, 1.f, 1.f }, "AIM");
    else
        ImGui::TextDisabled("aim");

    ImGui::Separator();

    float leftPanelWidth = ImGui::GetContentRegionAvail().x * 0.38f;

    if (ImGui::BeginChild("##InteractableListRegion",
        ImVec2(leftPanelWidth, 0), true, ImGuiWindowFlags_HorizontalScrollbar))
    {
        DrawInteractableList(interactables);
        ImGui::EndChild();
    }

    ImGui::SameLine();

    if (ImGui::BeginChild("##InteractableDetailsRegion", ImVec2(0, 0), true))
    {
        DrawEngineInteractionPanel(interaction);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        auto it = std::find_if(interactables.begin(), interactables.end(),
            [this](const AIInteractable& i) {
                return i.Handle == m_SelectedHandle;
            });

        if (it != interactables.end())
            DrawSelectedDetails(*it);
        else
            ImGui::TextDisabled("Select an interactable to view its AI perception state.");

        ImGui::EndChild();
    }
}

void UI_Interactable::Cleanup()
{
    m_SelectedHandle = 0xFFFFFFFF;
}

void UI_Interactable::DrawEngineInteractionPanel(const LiveInteraction& interaction)
{
    ImGui::TextColored({ 0.8f, 0.8f, 0.8f, 1.f }, "Engine Interaction State");
    ImGui::Spacing();

    bool hasObject = interaction.TargetObjectHandle != 0xFFFFFFFF
        && interaction.TargetObjectHandle != 0;

    if (hasObject)
    {
        ImGui::TextColored({ 0.2f, 1.f, 0.2f, 1.f }, "Object");
        ImGui::Text("  Type:   %s", InteractionTypeToString(interaction.Type));
        ImGui::Text("  Handle: 0x%08X", interaction.TargetObjectHandle);
        ImGui::Text("  Detail: %s", this->InteractionDetailToString(interaction.Type, interaction.InteractionSlotID));
    }
    else
    {
        ImGui::TextDisabled("Object — none");
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
        ImGui::TextDisabled("Melee — none");
    }

    ImGui::Spacing();

    bool hasAim = interaction.IsAimAvailable == 0x01
        && interaction.AimTargetHandle != 0xFFFFFFFF;

    if (hasAim)
    {
        ImGui::TextColored({ 0.4f, 0.8f, 1.f, 1.f }, "Aim");
        ImGui::Text("  Target:   0x%08X", interaction.AimTargetHandle);
        ImGui::Text("  SlotID:   0x%08X", interaction.AimTargetSlotID);
        ImGui::Text("  BodyPart: %s", BodyPartToString(interaction.BodyPart));
        ImGui::Text("  LocalPos: %.3f  %.3f  %.3f",
            interaction.AimHitLocalPosition[0],
            interaction.AimHitLocalPosition[1],
            interaction.AimHitLocalPosition[2]);
    }
    else
    {
        ImGui::TextDisabled("Aim — none");
    }
}

void UI_Interactable::DrawInteractableList(
    const std::vector<AIInteractable>& interactables)
{
    // Build handle set for root filtering (children are drawn under their parent).
    // Since VehicleParts are in ChildHandles of their root Vehicle, we collect
    // all child handles across all interactables.
    std::unordered_set<uint32_t> childHandleSet;
    std::unordered_map<uint32_t, const AIInteractable*> byHandle;

    byHandle.reserve(interactables.size());
    for (const auto& item : interactables)
    {
        byHandle[item.Handle] = &item;
        for (uint32_t ch : item.ChildHandles)
            childHandleSet.insert(ch);
    }

    for (const auto& item : interactables)
    {
        // Skip items that are children of another interactable.
        if (childHandleSet.count(item.Handle)) continue;

        const bool hasChildren = !item.ChildHandles.empty();
        const bool isSelected = (m_SelectedHandle == item.Handle);

        // Resolve TagName from ObjectTable.
        const std::string tagName = this->GetTagName(item.Handle);

        ImGui::PushStyleColor(ImGuiCol_Text, GetItemColor(item));

        std::string label = std::format("[{:.1f}m] {} | {}{}##{:08X}",
            item.DistanceToPlayer,
            RoleToString(item.Role),
            GetShortName(tagName),
            hasChildren ? " +" : "",
            item.Handle);

        if (ImGui::Selectable(label.c_str(), isSelected))
            m_SelectedHandle = item.Handle;

        ImGui::PopStyleColor();

        // Children (VehicleParts) immediately below, indented.
        if (hasChildren)
        {
            ImGui::Indent(14.0f);
            for (uint32_t childHandle : item.ChildHandles)
            {
                auto it = byHandle.find(childHandle);
                if (it == byHandle.end()) continue;

                const AIInteractable& child = *it->second;
                const bool            childSel = (m_SelectedHandle == child.Handle);
                const std::string     childTag = this->GetTagName(child.Handle);

                ImGui::PushStyleColor(ImGuiCol_Text, GetItemColor(child));

                std::string childLabel = std::format("[{:.1f}m] {} | {}##{:08X}",
                    child.DistanceToPlayer,
                    RoleToString(child.Role),
                    GetShortName(childTag),
                    child.Handle);

                if (ImGui::Selectable(childLabel.c_str(), childSel))
                    m_SelectedHandle = child.Handle;

                ImGui::PopStyleColor();
            }
            ImGui::Unindent(14.0f);
        }
    }
}

void UI_Interactable::DrawSelectedDetails(const AIInteractable& item)
{
    // Resolve LiveObject once for this frame.
    const LiveObject* objPtr =
        g_pState->Domain->ObjectTable->GetLiveObject(item.Handle);

    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f);

    if (ImGui::BeginChild("InteractableDetailCard", ImVec2(0, 0), true))
    {
        // --- Identity ---
        ImGui::PushStyleColor(ImGuiCol_Text, GetRoleColor(item.Role));
        if (objPtr)
            ImGui::TextWrapped("%s", objPtr->TagName.c_str());
        else
            ImGui::TextWrapped("0x%08X", item.Handle);
        ImGui::PopStyleColor();

        ImGui::Text("Handle: 0x%08X", item.Handle);
        ImGui::Text("Role:   %s", RoleToString(item.Role));

        if (objPtr && objPtr->ParentHandle != 0xFFFFFFFF)
            ImGui::Text("Parent: 0x%08X", objPtr->ParentHandle);

        ImGui::Spacing();
        ImGui::Separator();

        // --- Interaction state ---
        ImGui::Spacing();
        if (item.IsEngineSelected)
            ImGui::TextColored({ 0.2f, 1.f, 0.2f, 1.f },
                "[ ACTION READY — ENGINE SELECTED ]");
        else if (item.Activation == InteractableActivation::None)
            ImGui::TextColored({ 0.6f, 0.6f, 0.6f, 1.f },
                "[ WORLD ENTITY — NO DIRECT INTERACTION ]");
        else
            ImGui::TextColored({ 1.f, 0.4f, 0.4f, 1.f }, "[ OUT OF RANGE ]");

        ImGui::Spacing();
        ImGui::Separator();

        // --- Classification ---
        ImGui::Spacing();
        ImGui::TextColored({ 0.8f, 0.8f, 0.8f, 1.f }, "Classification");
        ImGui::Text("Activation: %s", ActivationToString(item.Activation));

        ImGui::Text("Behaviors:  ");
        for (size_t i = 0; i < item.Behaviors.size(); ++i)
        {
            ImGui::SameLine();
            if (i > 0) { ImGui::TextDisabled("|"); ImGui::SameLine(); }
            ImGui::TextDisabled("%s", BehaviorToString(item.Behaviors[i]));
        }

        // --- Spatial data ---
        if (objPtr)
        {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextColored({ 0.8f, 0.8f, 0.8f, 1.f }, "Spatial Data");

            ImGui::Text("Distance: %.2f m", item.DistanceToPlayer);
            ImGui::Text("Position: %.3f  %.3f  %.3f",
                objPtr->Position[0], objPtr->Position[1], objPtr->Position[2]);
            ImGui::Text("Forward:  %.3f  %.3f  %.3f",
                objPtr->Forward[0], objPtr->Forward[1], objPtr->Forward[2]);
            ImGui::Text("Up:       %.3f  %.3f  %.3f",
                objPtr->Up[0], objPtr->Up[1], objPtr->Up[2]);
            ImGui::Text("LinVel:   %.3f  %.3f  %.3f",
                objPtr->LinearVelocity[0],
                objPtr->LinearVelocity[1],
                objPtr->LinearVelocity[2]);
            ImGui::Text("AngVel:   %.3f  %.3f  %.3f",
                objPtr->AngularVelocity[0],
                objPtr->AngularVelocity[1],
                objPtr->AngularVelocity[2]);
        }

        // --- Object Profile ---
        if (objPtr)
        {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextColored({ 0.8f, 0.8f, 0.8f, 1.f }, "Object Profile");

            const ObjectProfile& p = objPtr->Profile;

            auto BoolBadge = [](const char* label, bool value)
                {
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
                ImGui::TableNextColumn(); BoolBadge("HasJmad:", p.HasJmad);
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
                        ImGui::TextDisabled("—");
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
            {
                const std::string childTag = this->GetTagName(ch);
                ImGui::Text("  0x%08X  %s", ch, GetShortName(childTag).c_str());
            }
        }

        ImGui::Spacing();
    }

    ImGui::EndChild();
    ImGui::PopStyleVar();
}

const char* UI_Interactable::InteractionDetailToString(InteractionType type, InteractionDetail detail)
{
    switch (type)
    {
    case InteractionType::GrabWeapon:
        switch (detail)
        {
        case InteractionDetail::GrabWeapon:    return "Grab Weapon";
        case InteractionDetail::ChangeWeapon:  return "Change Weapon";
        default:                               return "Unknown";
        }

    case InteractionType::EnterVehicle:
    case InteractionType::Hijack:
        switch (detail)
        {
        case InteractionDetail::ZeroSeat:      return "Seat (0)";
        case InteractionDetail::FirstSeat:     return "Seat (1)";
        case InteractionDetail::SecondSeat:    return "Seat (2)";
        case InteractionDetail::ThirdSeat:     return "Seat (3)";
        case InteractionDetail::FourthSeat:    return "Seat (4)";
        case InteractionDetail::FifthSeat:     return "Seat (5)";
        default:                               return "Unknown Seat";
        }

    case InteractionType::GrabArmorAbility:    return "Grab Armor Ability";
    case InteractionType::TakeHealthStation:   return "Take Health Station";
    case InteractionType::GrabObjective:       return "Grab Objective";
    case InteractionType::None:                return "None";
    default:                                   return "Unknown";
    }
}

std::string UI_Interactable::GetShortName(const std::string& fullPath) const
{
    size_t pos = fullPath.find_last_of("\\/");
    return pos != std::string::npos ? fullPath.substr(pos + 1) : fullPath;
}

std::string UI_Interactable::GetTagName(uint32_t handle) const
{
    const LiveObject* obj = g_pState->Domain->ObjectTable->GetLiveObject(handle);
    return obj ? obj->TagName : std::format("0x{:08X}", handle);
}