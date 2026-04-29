#include "pch.h"
#include "Core/States/Core_State.h"
#include "Core/States/Domain/Core_State_Domain.h"
#include "Core/States/Domain/Tables/State_InteractionTable.h"
#include "Core/States/Domain/Interactable/State_Interactable.h"
#include "Core/UI/Domain/Interactable/UI_Interactable.h"
#include "External/imgui/imgui.h"
#include <Algorithm>
#include <format>

// Helpers.
namespace
{
    const char* TagGroupToString(TagGroup group)
    {
        switch (group)
        {
        case TagGroup::Weapon:       return "Weapon";
        case TagGroup::Equipment:    return "Equipment";
        case TagGroup::Vehicle:      return "Vehicle";
        case TagGroup::DeviceControl:return "Control";
        default:                     return "Unknown";
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

    ImVec4 GetGroupColor(TagGroup group)
    {
        switch (group)
        {
        case TagGroup::Vehicle:       return { 1.0f, 0.8f, 0.2f, 1.0f }; // yellow
        case TagGroup::Weapon:        return { 0.4f, 0.8f, 1.0f, 1.0f }; // sky blue
        case TagGroup::Equipment:     return { 0.6f, 1.0f, 0.6f, 1.0f }; // light green
        case TagGroup::DeviceControl: return { 1.0f, 0.5f, 0.8f, 1.0f }; // pink
        default:                      return { 0.7f, 0.7f, 0.7f, 1.0f };
        }
    }

    ImVec4 GetItemColor(const AIInteractable& item)
    {
        if (item.IsEngineSelected) return { 0.2f, 1.0f, 0.2f, 1.0f };
        return GetGroupColor(item.PrimaryGroup());
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
                return i.Handle() == m_SelectedHandle;
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

// Engine interaction panel (global, always visible)
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
        ImGui::Text("  SlotID: %u", interaction.InteractionSlotID);
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

// Left list.
void UI_Interactable::DrawInteractableList(
    const std::vector<AIInteractable>& interactables)
{
    // Quick lookup handle -> interactive.
    std::unordered_map<uint32_t, const AIInteractable*> byHandle;
    byHandle.reserve(interactables.size());
    for (const auto& item : interactables)
        byHandle[item.Handle()] = &item;

    for (const auto& item : interactables)
    {
        // Solo roots, the children are drawn below their father.
        if (byHandle.count(item.ParentHandle()) > 0) continue;

        bool hasChildren = !item.ChildHandles.empty();
        bool isSelected = (m_SelectedHandle == item.Handle());

        ImGui::PushStyleColor(ImGuiCol_Text, GetItemColor(item));

        std::string label = std::format("[{:.1f}m] {} | {}{}##{:08X}",
            item.DistanceToPlayer,
            TagGroupToString(item.PrimaryGroup()),
            GetShortName(item.TagName()),
            hasChildren ? " +" : "",
            item.Handle());

        if (ImGui::Selectable(label.c_str(), isSelected))
            m_SelectedHandle = item.Handle();

        ImGui::PopStyleColor();

        //Children (turrets, sub-vehicles) immediately below, indented.
        if (hasChildren)
        {
            ImGui::Indent(14.0f);
            for (uint32_t childHandle : item.ChildHandles)
            {
                auto it = byHandle.find(childHandle);
                if (it == byHandle.end()) continue;

                const AIInteractable& child = *it->second;
                bool                  childSel = (m_SelectedHandle == child.Handle());

                ImGui::PushStyleColor(ImGuiCol_Text, GetItemColor(child));

                std::string childLabel = std::format("[{:.1f}m] {} | {}##{:08X}",
                    child.DistanceToPlayer,
                    TagGroupToString(child.PrimaryGroup()),
                    GetShortName(child.TagName()),
                    child.Handle());

                if (ImGui::Selectable(childLabel.c_str(), childSel))
                    m_SelectedHandle = child.Handle();

                ImGui::PopStyleColor();
            }
            ImGui::Unindent(14.0f);
        }
    }
}

// Details panel.
void UI_Interactable::DrawSelectedDetails(const AIInteractable& item)
{
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f);

    if (ImGui::BeginChild("InteractableDetailCard", ImVec2(0, 0), true))
    {
        // Identity.
        ImGui::PushStyleColor(ImGuiCol_Text, GetGroupColor(item.PrimaryGroup()));
        ImGui::TextWrapped("%s", item.TagName().c_str());
        ImGui::PopStyleColor();

        ImGui::Text("Handle:  0x%08X", item.Handle());
        if (item.ParentHandle() != 0xFFFFFFFF)
            ImGui::Text("Parent:  0x%08X", item.ParentHandle());

        ImGui::Spacing();
        ImGui::Separator();

        // Interaction state.
        ImGui::Spacing();
        if (item.IsEngineSelected)
            ImGui::TextColored({ 0.2f, 1.f, 0.2f, 1.f }, "[ ACTION READY — ENGINE SELECTED ]");
        else if (item.Activation == InteractableActivation::None)
            ImGui::TextColored({ 0.6f, 0.6f, 0.6f, 1.f }, "[ WORLD ENTITY — NO DIRECT INTERACTION ]");
        else
            ImGui::TextColored({ 1.f, 0.4f, 0.4f, 1.f }, "[ OUT OF RANGE ]");

        ImGui::Spacing();
        ImGui::Separator();

        // Classification.
        ImGui::Spacing();
        ImGui::TextColored({ 0.8f, 0.8f, 0.8f, 1.f }, "Classification");
        ImGui::Text("Group:      %s", TagGroupToString(item.PrimaryGroup()));
        ImGui::Text("Activation: %s", ActivationToString(item.Activation));

        ImGui::Text("Behaviors:  ");
        for (size_t i = 0; i < item.Behaviors.size(); ++i)
        {
            ImGui::SameLine();
            if (i > 0) { ImGui::TextDisabled("|"); ImGui::SameLine(); }
            ImGui::TextDisabled("%s", BehaviorToString(item.Behaviors[i]));
        }

        ImGui::Spacing();
        ImGui::Separator();

        // Tag Profile.
        ImGui::Spacing();
        ImGui::TextColored({ 0.8f, 0.8f, 0.8f, 1.f }, "Tag Profile");

        const TagProfile& p = item.Node.Profile;

        auto BoolBadge = [](const char* label, bool value)
            {
                ImGui::Text("%-10s", label);
                ImGui::SameLine();
                if (value) ImGui::TextColored({ 0.4f, 1.f, 0.4f, 1.f }, "yes");
                else       ImGui::TextColored({ 0.4f, 0.4f, 0.4f, 1.f }, "no");
            };

        // Two columns for compacting.
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

        ImGui::Spacing();
        ImGui::Separator();

        // Spatial Data.
        ImGui::Spacing();
        ImGui::TextColored({ 0.8f, 0.8f, 0.8f, 1.f }, "Spatial Data");
        ImGui::Text("Distance: %.2f m", item.DistanceToPlayer);
        ImGui::Text("Position: %.3f  %.3f  %.3f",
            item.Node.Position[0], item.Node.Position[1], item.Node.Position[2]);
        ImGui::Text("Forward:  %.3f  %.3f  %.3f",
            item.Node.Forward[0], item.Node.Forward[1], item.Node.Forward[2]);
        ImGui::Text("Up:       %.3f  %.3f  %.3f",
            item.Node.Up[0], item.Node.Up[1], item.Node.Up[2]);

        // Velocity
        // ObjectNode doesn't have velocity yet, but LiveObject does.
        // TODO: Add velocity to ObjectNode.

        // Vehicle Seats.
        if (!item.Seats.empty())
        {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextColored({ 1.f, 0.8f, 0.2f, 1.f }, "Vehicle Seats (%zu)", item.Seats.size());

            if (ImGui::BeginTable("##seats", 4,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
            {
                ImGui::TableSetupColumn("Seat");
                ImGui::TableSetupColumn("Status");
                ImGui::TableSetupColumn("Occupant");
                ImGui::TableSetupColumn("Dist");
                ImGui::TableHeadersRow();

                for (const auto& seat : item.Seats)
                {
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", seat.SeatName.c_str());

                    ImGui::TableSetColumnIndex(1);
                    if (seat.IsOccupied)
                        ImGui::TextColored({ 1.f, 0.4f, 0.4f, 1.f }, "Occupied");
                    else
                        ImGui::TextColored({ 0.4f, 1.f, 0.4f, 1.f }, "Free");

                    ImGui::TableSetColumnIndex(2);
                    if (seat.OccupyingBipedHandle == 0xFFFFFFFF)
                        ImGui::TextDisabled("—");
                    else
                        ImGui::Text("0x%08X", seat.OccupyingBipedHandle);

                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%.2f m", seat.DistanceToPlayer);
                }

                ImGui::EndTable();
            }
        }

        // Child Vehicles.
        if (!item.ChildHandles.empty())
        {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextColored({ 1.f, 0.8f, 0.2f, 1.f },
                "Child Vehicles (%zu)", item.ChildHandles.size());

            for (uint32_t ch : item.ChildHandles)
                ImGui::Text("  0x%08X", ch);
        }

        ImGui::Spacing();
    }

    ImGui::EndChild();
    ImGui::PopStyleVar();
}

std::string UI_Interactable::GetShortName(const std::string& fullPath) const
{
    size_t pos = fullPath.find_last_of("\\/");
    return pos != std::string::npos ? fullPath.substr(pos + 1) : fullPath;
}