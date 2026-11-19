module;

#include "External/imgui/imgui.h"

module DevTools.Viewer.Inspector.System;
import :Interaction;

import Common.Engine.Type;
import Common.Semantic.Type;
import Blam.Runtime.Object.Type;
import Views.Affordance.Type;
import DevTools.Gui.Color.System;
import DevTools.Gui.Widget.System;
import DevTools.Viewer.Format.System;
import std;

namespace
{
    using Bridge::Codec::Tick::Type::Tick;
    using Blam::Runtime::Object::Type::Alive::ObjectTable;
    using Blam::Runtime::Object::Type::Alive::AliveObject;
    using Views::Affordance::Type::Affordance;
    using Views::Affordance::Type::AffordanceActivation;
    using Views::Affordance::Type::SeatStatus;
    using Blam::Runtime::Object::Type::Profile::Profile;

    using Common::Semantic::Type::GroupOf;
    using Common::Semantic::Type::NameOf;

    using DevTools::Gui::Color::System::GroupToColor;
    using DevTools::Viewer::Format::System::AffordanceFormater;

    using DevTools::Gui::Widget::System::PanelGuiService;

    using Common::Engine::Type::k_NoHandle;

    constexpr ImVec4 k_PositiveColor{ 0.4f, 1.0f, 0.4f, 1.0f };

    auto FindObject(const ObjectTable* objects, std::uint32_t handle) -> const AliveObject*
    {
        if (!objects) return nullptr;

        auto it = objects->find(handle);
        return (it != objects->end()) ? &it->second : nullptr;
    }

    auto ShortTag(const ObjectTable* objects, std::uint32_t handle) -> std::string
    {
        const AliveObject* object = FindObject(objects, handle);
        if (!object) return {};

        const std::string& tag = object->TagName;
        const std::size_t slash = tag.find_last_of("\\/");

        return (slash == std::string::npos) ? tag : tag.substr(slash + 1);
    }

    auto DrawProfile(const Profile& p) -> void
    {
        PanelGuiService::DrawSectionSeparator("Object Profile");

        if (!ImGui::BeginTable("##profile", 2)) return;

        const std::pair<const char*, bool> badges[] = {
            { "HasColl:", p.HasColl }, { "HasHlmt:", p.HasHlmt },
            { "HasMode:", p.HasMode },
        };

        for (const auto& [label, value] : badges)
        {
            ImGui::TableNextColumn();
            PanelGuiService::DrawBoolBadge(label, value, k_PositiveColor);
        }

        ImGui::EndTable();
    }

    auto DrawSeats(const std::vector<SeatStatus>& seats) -> void
    {
        PanelGuiService::DrawSectionSeparator(nullptr);
        ImGui::TextColored({ 1.f, 0.8f, 0.2f, 1.f }, "Vehicle Seats (%zu)", seats.size());

        if (!ImGui::BeginTable("##seats", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            return;
        }

        ImGui::TableSetupColumn("Seat");
        ImGui::TableSetupColumn("Type");
        ImGui::TableSetupColumn("Status");
        ImGui::TableSetupColumn("Occupant");
        ImGui::TableSetupColumn("Dist");
        ImGui::TableHeadersRow();

        for (const SeatStatus& seat : seats)
        {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", seat.SeatName.c_str());

            ImGui::TableSetColumnIndex(1);
            if (seat.IsHijackerSlot) ImGui::TextColored({ 1.f, 0.6f, 0.1f, 1.f }, "Hijack");
            else                     ImGui::TextDisabled("Normal");

            ImGui::TableSetColumnIndex(2);
            if (seat.IsOccupied) ImGui::TextColored({ 1.f, 0.4f, 0.4f, 1.f }, "Occupied");
            else                 ImGui::TextColored({ 0.4f, 1.f, 0.4f, 1.f }, "Free");

            ImGui::TableSetColumnIndex(3);
            if (seat.OccupyingBipedHandle == k_NoHandle) ImGui::TextDisabled("-");
            else ImGui::Text("0x%08X", seat.OccupyingBipedHandle);

            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%.2f m", seat.DistanceToPlayer);
        }

        ImGui::EndTable();
    }

    auto DrawDetails(const Affordance& item, const ObjectTable* objects) -> void
    {
        const AliveObject* object = FindObject(objects, item.Handle);

        // --- Identity ---
        ImGui::PushStyleColor(ImGuiCol_Text, GroupToColor::FromGroup(GroupOf(item.Function)));
        if (object) ImGui::TextWrapped("%s", object->TagName.c_str());
        else        ImGui::TextWrapped("0x%08X", item.Handle);
        ImGui::PopStyleColor();

        ImGui::Text("Handle: 0x%08X", item.Handle);
        const std::string function{ NameOf(item.Function) };
        ImGui::Text("Function: %s", function.c_str());

        if (object && object->ParentHandle != k_NoHandle)
        {
            ImGui::Text("Parent: 0x%08X", object->ParentHandle);
        }

        // --- Interaction state ---
        PanelGuiService::DrawSectionSeparator(nullptr);

        if (item.IsEngineSelected)
            ImGui::TextColored({ 0.2f, 1.f, 0.2f, 1.f }, "[ ACTION READY - ENGINE SELECTED ]");
        else if (item.Activation == AffordanceActivation::None)
            ImGui::TextColored({ 0.6f, 0.6f, 0.6f, 1.f }, "[ WORLD ENTITY - NO DIRECT INTERACTION ]");
        else
            ImGui::TextColored({ 1.f, 0.4f, 0.4f, 1.f }, "[ OUT OF RANGE ]");

        // --- Classification ---
        PanelGuiService::DrawSectionSeparator("Classification");

        ImGui::Text("Activation: %s", AffordanceFormater::ActivationToString(item.Activation));
        ImGui::Text("Behaviors:  ");

        for (std::size_t i = 0; i < item.Behaviors.size(); ++i)
        {
            ImGui::SameLine();
            if (i > 0) { ImGui::TextDisabled("|"); ImGui::SameLine(); }
            ImGui::TextDisabled("%s", AffordanceFormater::BehaviorToString(item.Behaviors[i]));
        }

        // --- Spatial data ---
        PanelGuiService::DrawSectionSeparator("Spatial Data");

        ImGui::Text("Distance: %.2f m", item.DistanceToPlayer);
        PanelGuiService::DrawVec3("Position:", item.Position);

        if (object)
        {
            PanelGuiService::DrawVec3("Forward: ", object->Forward);
            PanelGuiService::DrawVec3("Up:      ", object->Up);
            PanelGuiService::DrawVec3("LinVel:  ", object->LinearVelocity);
            PanelGuiService::DrawVec3("AngVel:  ", object->AngularVelocity);

            DrawProfile(object->Profile);
        }

        // --- Vehicle Seats ---
        if (!item.Seats.empty()) DrawSeats(item.Seats);

        // --- Child Vehicles ---
        if (!item.ChildHandles.empty())
        {
            PanelGuiService::DrawSectionSeparator(nullptr);
            ImGui::TextColored({ 1.f, 0.8f, 0.2f, 1.f },
                "Child Vehicles (%zu)", item.ChildHandles.size());

            for (const std::uint32_t child : item.ChildHandles)
            {
                ImGui::Text("  0x%08X  %s", child, ShortTag(objects, child).c_str());
            }
        }
    }
}

namespace DevTools::Viewer::Inspector::System
{
    auto InteractionSection::Find(const Tick& tick, std::uint32_t handle) -> const Affordance*
    {
        if (!tick.Affordances) return nullptr;

        for (const Affordance& item : *tick.Affordances)
        {
            if (item.Handle == handle) return &item;
        }

        return nullptr;
    }

    auto InteractionSection::Draw(const Tick& tick, const Affordance* item) -> void
    {
        if (!item)
        {
            ImGui::TextDisabled("No affordance for this object.");
            return;
        }

        DrawDetails(*item, tick.ObjectTable.get());
    }
}