#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Domain/Tables/InteractionTableState.h"
#include "Core/States/Domain/Interactable/InteractableState.h"
#include "Core/States/Domain/Tags/Phmo/PhmoState.h"
#include "Core/UI/Tabs/Domain/Interactable/InteractableTab.h"
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
        case TagGroup::Vehicle:      return "Vehicle";
        case TagGroup::Weapon:       return "Weapon";
        case TagGroup::Equipment:    return "Equipment";
        case TagGroup::Scenery:      return "Scenery";
        case TagGroup::Control:      return "Control";
        case TagGroup::Crate:        return "Crate";
        case TagGroup::Projectile:   return "Projectile";
        default:                     return "Unknown";
        }
    }

    const char* BehaviorToString(InteractableBehavior behavior)
    {
        switch (behavior)
        {
        case InteractableBehavior::Pickup:        return "Pickup";
        case InteractableBehavior::EnterVehicle:  return "EnterVehicle";
        case InteractableBehavior::Avoid:         return "Avoid";
        case InteractableBehavior::Interact:      return "Interact";
        default:                                  return "Unknown";
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
        case InteractionType::None:             return "None";
        case InteractionType::GrabWeapon:       return "GrabWeapon";
        case InteractionType::GrabArmorAbility: return "GrabArmorAbility";
        case InteractionType::TakeHealthStation:return "TakeHealthStation";
        case InteractionType::EnterVehicle:     return "EnterVehicle";
        case InteractionType::Hijack:           return "Hijack";
        case InteractionType::GrabObjective:    return "GrabObjective";
        default:                                return "Unknown";
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

    // Color by state of the interactable in the list.
    ImVec4 GetItemColor(const AIInteractable& item)
    {
        if (item.IsEngineSelected) return { 0.2f, 1.0f, 0.2f, 1.0f };
        return { 1.0f, 1.0f, 1.0f, 1.0f };
    }
}

void InteractableTab::Draw()
{
    std::vector<AIInteractable> interactables =
        g_pState->Domain->Interactable->GetInteractables();

    std::sort(interactables.begin(), interactables.end(),
        [](const AIInteractable& a, const AIInteractable& b) {
            return a.DistanceToPlayer < b.DistanceToPlayer;
        });

    const LiveInteraction interaction =
        g_pState->Domain->InteractionTable->GetLiveInteraction();

    ImGui::TextDisabled("Updated interactables: %zu", interactables.size());

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
        this->DrawInteractableList(interactables);
        ImGui::EndChild();
    }

    ImGui::SameLine();

    if (ImGui::BeginChild("##InteractableDetailsRegion", ImVec2(0, 0), true))
    {
        // Global engine state panel always visible at the top.
        this->DrawEngineInteractionPanel(interaction);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        auto it = std::find_if(interactables.begin(), interactables.end(),
            [this](const AIInteractable& i) {
                return i.Handle == m_SelectedHandle;
            });

        if (it != interactables.end())
            this->DrawSelectedDetails(*it);
        else
            ImGui::TextDisabled("Select an interactable to view its AI perception state.");

        ImGui::EndChild();
    }
}

void InteractableTab::Cleanup()
{
    m_SelectedHandle = 0xFFFFFFFF;
}

// Engine interaction panel (global, always visible)
void InteractableTab::DrawEngineInteractionPanel(const LiveInteraction& interaction)
{
    ImGui::TextColored({ 0.8f, 0.8f, 0.8f, 1.f }, "Engine Interaction State");
    ImGui::Spacing();

    // Object interaction.
    bool hasObject = interaction.TargetObjectHandle != 0xFFFFFFFF
        && interaction.TargetObjectHandle != 0;

    if (hasObject)
    {
        ImGui::TextColored({ 0.2f, 1.f, 0.2f, 1.f }, "Object");
        ImGui::Text("Type: %s", InteractionTypeToString(interaction.Type));
        ImGui::Text("Handle: 0x%08X", interaction.TargetObjectHandle);
        ImGui::Text("SlotID: %u", interaction.InteractionSlotID);
    }
    else
    {
        ImGui::TextDisabled("Object — none");
    }

    ImGui::Spacing();

    // Melee.
    bool hasMelee = interaction.IsMeleeAvailable == 0x0E
        && interaction.MeleeTargetHandle != 0xFFFFFFFF;

    if (hasMelee)
    {
        ImGui::TextColored({ 1.f, 0.4f, 0.2f, 1.f }, "Melee");
        ImGui::Text("Target: 0x%08X", interaction.MeleeTargetHandle);
    }
    else
    {
        ImGui::TextDisabled("Melee — none");
    }

    ImGui::Spacing();

    // Aim.
    bool hasAim = interaction.IsAimAvailable == 0x01
        && interaction.AimTargetHandle != 0xFFFFFFFF;

    if (hasAim)
    {
        ImGui::TextColored({ 0.4f, 0.8f, 1.f, 1.f }, "Aim");
        ImGui::Text("Target: 0x%08X", interaction.AimTargetHandle);
        ImGui::Text("SlotID: 0x%08X", interaction.AimTargetSlotID);
        ImGui::Text("BodyPart: %s", BodyPartToString(interaction.BodyPart));
        ImGui::Text("LocalPos: %.3f %.3f %.3f",
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
void InteractableTab::DrawInteractableList(
    const std::vector<AIInteractable>& interactables)
{
    // Build a lookup for quick child resolution.
    std::unordered_map<uint32_t, const AIInteractable*> byHandle;
    byHandle.reserve(interactables.size());
    for (const auto& item : interactables)
        byHandle[item.Handle] = &item;

    for (const auto& item : interactables)
    {
        // Skip children, drawn right after their parent.
        if (byHandle.count(item.ParentHandle) > 0) continue;

        bool hasChildren = !item.ChildHandles.empty();
        bool isSelected = (m_SelectedHandle == item.Handle);

        ImGui::PushStyleColor(ImGuiCol_Text, GetItemColor(item));

        // Parent label, append a small marker if it has children.
        std::string label = hasChildren
            ? std::format("[{:.1f}m] {} | {} ##{:08X}",
                item.DistanceToPlayer,
                TagGroupToString(item.PrimaryGroup),
                GetShortName(item.TagName),
                item.Handle)
            : std::format("[{:.1f}m] {} | {}##{:08X}",
                item.DistanceToPlayer,
                TagGroupToString(item.PrimaryGroup),
                GetShortName(item.TagName),
                item.Handle);

        if (ImGui::Selectable(label.c_str(), isSelected))
            m_SelectedHandle = item.Handle;

        ImGui::PopStyleColor();

        // Draw children immediately after, indented.
        if (hasChildren)
        {
            ImGui::Indent(12.0f);

            for (uint32_t childHandle : item.ChildHandles)
            {
                auto it = byHandle.find(childHandle);
                if (it == byHandle.end()) continue;

                const AIInteractable& child = *it->second;
                bool                  childSel = (m_SelectedHandle == child.Handle);

                ImGui::PushStyleColor(ImGuiCol_Text, GetItemColor(child));

                std::string childLabel = std::format("[{:.1f}m] {} | {}##{:08X}",
                    child.DistanceToPlayer,
                    TagGroupToString(child.PrimaryGroup),
                    GetShortName(child.TagName),
                    child.Handle);

                if (ImGui::Selectable(childLabel.c_str(), childSel))
                    m_SelectedHandle = child.Handle;

                ImGui::PopStyleColor();
            }

            ImGui::Unindent(12.0f);
        }
    }
}

// Details panel.
void InteractableTab::DrawSelectedDetails(const AIInteractable& interactable)
{
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f);

    if (ImGui::BeginChild("InteractableDetailCard", ImVec2(0, 0), true))
    {
        // Identity.
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
        ImGui::TextWrapped("%s", interactable.TagName.c_str());
        ImGui::PopStyleColor();
        ImGui::Text("Handle: 0x%08X", interactable.Handle);

        ImGui::Spacing();
        ImGui::Separator();

        // Interaction state.
        ImGui::Spacing();
        if (interactable.Activation == InteractableActivation::None)
        {
            ImGui::TextColored({ 0.6f, 0.6f, 0.6f, 1.f },
                "[ WORLD ENTITY — NO DIRECT INTERACTION ]");
        }
        else if (interactable.IsEngineSelected)
        {
            ImGui::TextColored({ 0.2f, 1.f, 0.2f, 1.f },
                "[ ACTION READY — ENGINE SELECTED ]");
        }
        else
        {
            ImGui::TextColored({ 1.f, 0.4f, 0.4f, 1.f },
                "[ OUT OF RANGE ]");
        }

        ImGui::Spacing();
        ImGui::Separator();

        // Classification.
        ImGui::Spacing();
        ImGui::TextColored({ 0.8f, 0.8f, 0.8f, 1.f }, "Classification");
        ImGui::Text("Group: %s", TagGroupToString(interactable.PrimaryGroup));
        ImGui::Text("Activation: %s", ActivationToString(interactable.Activation));
        ImGui::Text("Behaviors: ");
        ImGui::SameLine();
        for (size_t i = 0; i < interactable.Behaviors.size(); ++i)
        {
            if (i > 0) { ImGui::SameLine(); ImGui::TextDisabled(" |"); ImGui::SameLine(); }
            ImGui::TextDisabled("%s", BehaviorToString(interactable.Behaviors[i]));
        }

        ImGui::Spacing();
        ImGui::Separator();

        // TagProfile.
        ImGui::Spacing();
        ImGui::TextColored({ 0.8f, 0.8f, 0.8f, 1.f }, "Tag Profile");

        auto BoolText = [](const char* label, bool value)
            {
                ImGui::Text("%-14s", label);
                ImGui::SameLine();
                if (value) ImGui::TextColored({ 0.4f, 1.f, 0.4f, 1.f }, "true");
                else       ImGui::TextColored({ 0.5f, 0.5f, 0.5f, 1.f }, "false");
            };

        const TagProfile& p = interactable.Profile;
        BoolText("HasPhmo:", p.HasPhmo);
        BoolText("HasColl:", p.HasColl);
        BoolText("HasVehi:", p.HasVehi);
        BoolText("HasWeap:", p.HasWeap);

        ImGui::Spacing();
        ImGui::Separator();

        // Spatial data.
        ImGui::Spacing();
        ImGui::TextColored({ 0.8f, 0.8f, 0.8f, 1.f }, "Spatial Data");
        ImGui::Text("Distance: %.2f m", interactable.DistanceToPlayer);
        ImGui::Text("Position: %.2f %.2f %.2f",
            interactable.Position[0], interactable.Position[1], interactable.Position[2]);
        ImGui::Text("Forward: %.2f %.2f %.2f",
            interactable.Forward[0], interactable.Forward[1], interactable.Forward[2]);
        ImGui::Text("Up: %.2f %.2f %.2f",
            interactable.Up[0], interactable.Up[1], interactable.Up[2]);

        // Seats.
        if (!interactable.Seats.empty())
        {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextColored({ 1.f, 0.8f, 0.2f, 1.f }, "Vehicle Seats");

            if (ImGui::BeginTable("SeatsTable", 4,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
            {
                ImGui::TableSetupColumn("Seat");
                ImGui::TableSetupColumn("Status");
                ImGui::TableSetupColumn("Occupant");
                ImGui::TableSetupColumn("Distance To Seat");
                ImGui::TableHeadersRow();

                for (const auto& seat : interactable.Seats)
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
                    ImGui::Text("%.2f", seat.DistanceToPlayer);
                }

                ImGui::EndTable();
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
    }

    ImGui::EndChild();
    ImGui::PopStyleVar();
}

std::string InteractableTab::GetShortName(const std::string& fullPath) const
{
    size_t pos = fullPath.find_last_of("\\/");
    return pos != std::string::npos ? fullPath.substr(pos + 1) : fullPath;
}