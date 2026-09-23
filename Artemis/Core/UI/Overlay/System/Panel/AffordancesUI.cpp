module;

#include "External/imgui/imgui.h"

module UI.Overlay.System;
import :Affordances;

import Common.Math.Type;
import Tables.Object.Type;
import Egocentric.Affordance.Type;
import UI.Color.System;
import UI.Format.System;
import std;

namespace
{
	using Tick = Export::Tick::Type::Tick;
	using ObjectTable = Export::Tick::Type::ObjectTable;
	using AliveObject = Export::Tick::Type::AliveObject;
	using Interaction = Export::Tick::Type::Interaction;
	using Affordance = Export::Tick::Type::Affordance;
	using Activation = Egocentric::Affordance::Type::Activation;
	using SeatStatus = Egocentric::Affordance::Type::SeatStatus;
	using Profile = Tables::Object::Type::Profile::Profile;
	using Vec3 = Common::Math::Type::Vec3;

	using RoleToColor = UI::Color::System::RoleToColor;
	using RoleFormater = UI::Format::System::RoleFormater;
	using AffordanceFormater = UI::Format::System::AffordanceFormater;
	using InteractionFormater = UI::Format::System::InteractionFormater;

	constexpr std::uint32_t k_InvalidHandle = 0xFFFFFFFF;
	constexpr std::uint8_t k_MeleeAvailable = 0x0E;
	constexpr std::uint8_t k_AimAvailable = 0x01;

	const ImVec4 k_SectionColor{ 0.8f, 0.8f, 0.8f, 1.0f };

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

	auto DrawVec3(const char* label, const Vec3& value) -> void
	{
		ImGui::Text("%s %.3f  %.3f  %.3f", label, value.X, value.Y, value.Z);
	}

	auto DrawSectionSeparator(const char* title) -> void
	{
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		if (title) ImGui::TextColored(k_SectionColor, "%s", title);
	}

	auto DrawEngineInteraction(const Interaction& interaction) -> void
	{
		ImGui::TextColored(k_SectionColor, "Engine Interaction State");
		ImGui::Spacing();

		const bool hasObject = interaction.TargetObjectHandle != k_InvalidHandle
			&& interaction.TargetObjectHandle != 0;

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

		const bool hasMelee = interaction.IsMeleeAvailable == k_MeleeAvailable
			&& interaction.MeleeTargetHandle != k_InvalidHandle;

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

		const bool hasAim = interaction.IsAimAvailable == k_AimAvailable
			&& interaction.AimTargetHandle != k_InvalidHandle;

		if (hasAim)
		{
			ImGui::TextColored({ 0.4f, 0.8f, 1.f, 1.f }, "Aim");
			ImGui::Text("  Target:    0x%08X", interaction.AimTargetHandle);
			ImGui::Text("  SlotID:    0x%08X", interaction.AimTargetSlotID);
			ImGui::Text("  ModelPart: 0x%02X", interaction.ModelPart);
			DrawVec3("  LocalPos: ", interaction.AimHitLocalPosition);
		}
		else
		{
			ImGui::TextDisabled("Aim - none");
		}
	}

	auto DrawBoolBadge(const char* label, bool value) -> void
	{
		ImGui::Text("%-10s", label);
		ImGui::SameLine();

		if (value) ImGui::TextColored({ 0.4f, 1.f, 0.4f, 1.f }, "yes");
		else       ImGui::TextColored({ 0.4f, 0.4f, 0.4f, 1.f }, "no");
	}

	auto DrawProfile(const Profile& p) -> void
	{
		DrawSectionSeparator("Object Profile");

		if (!ImGui::BeginTable("##profile", 2)) return;

		const std::pair<const char*, bool> badges[] = {
			{ "HasBipd:", p.HasBipd }, { "HasVehi:", p.HasVehi },
			{ "HasWeap:", p.HasWeap }, { "HasEqip:", p.HasEqip },
			{ "HasProj:", p.HasProj }, { "HasScen:", p.HasScen },
			{ "HasMach:", p.HasMach }, { "HasCtrl:", p.HasCtrl },
			{ "HasBloc:", p.HasBloc }, { "HasColl:", p.HasColl },
			{ "HasPhmo:", p.HasPhmo }, { "HasMode:", p.HasMode },
		};

		for (const auto& [label, value] : badges)
		{
			ImGui::TableNextColumn();
			DrawBoolBadge(label, value);
		}

		ImGui::EndTable();
	}

	auto DrawSeats(const std::vector<SeatStatus>& seats) -> void
	{
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
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
			if (seat.OccupyingBipedHandle == k_InvalidHandle) ImGui::TextDisabled("-");
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
		ImGui::PushStyleColor(ImGuiCol_Text, RoleToColor::FromRole(item.Role));
		if (object) ImGui::TextWrapped("%s", object->TagName.c_str());
		else        ImGui::TextWrapped("0x%08X", item.Handle);
		ImGui::PopStyleColor();

		ImGui::Text("Handle: 0x%08X", item.Handle);
		ImGui::Text("Role:   %s", RoleFormater::RoleToString(item.Role));

		if (object && object->ParentHandle != k_InvalidHandle)
		{
			ImGui::Text("Parent: 0x%08X", object->ParentHandle);
		}

		// --- Interaction state ---
		DrawSectionSeparator(nullptr);

		if (item.IsEngineSelected)
			ImGui::TextColored({ 0.2f, 1.f, 0.2f, 1.f }, "[ ACTION READY - ENGINE SELECTED ]");
		else if (item.Activation == Activation::None)
			ImGui::TextColored({ 0.6f, 0.6f, 0.6f, 1.f }, "[ WORLD ENTITY - NO DIRECT INTERACTION ]");
		else
			ImGui::TextColored({ 1.f, 0.4f, 0.4f, 1.f }, "[ OUT OF RANGE ]");

		// --- Classification ---
		DrawSectionSeparator("Classification");

		ImGui::Text("Activation: %s", AffordanceFormater::ActivationToString(item.Activation));
		ImGui::Text("Behaviors:  ");

		for (std::size_t i = 0; i < item.Behaviors.size(); ++i)
		{
			ImGui::SameLine();
			if (i > 0) { ImGui::TextDisabled("|"); ImGui::SameLine(); }
			ImGui::TextDisabled("%s", AffordanceFormater::BehaviorToString(item.Behaviors[i]));
		}

		// --- Spatial data ---
		DrawSectionSeparator("Spatial Data");

		ImGui::Text("Distance: %.2f m", item.DistanceToPlayer);
		DrawVec3("Position:", item.Position);

		if (object)
		{
			DrawVec3("Forward: ", object->Forward);
			DrawVec3("Up:      ", object->Up);
			DrawVec3("LinVel:  ", object->LinearVelocity);
			DrawVec3("AngVel:  ", object->AngularVelocity);

			DrawProfile(object->Profile);
		}

		// --- Vehicle Seats ---
		if (!item.Seats.empty()) DrawSeats(item.Seats);

		// --- Child Vehicles ---
		if (!item.ChildHandles.empty())
		{
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();
			ImGui::TextColored({ 1.f, 0.8f, 0.2f, 1.f },
				"Child Vehicles (%zu)", item.ChildHandles.size());

			for (const std::uint32_t child : item.ChildHandles)
			{
				ImGui::Text("  0x%08X  %s", child, ShortTag(objects, child).c_str());
			}
		}
	}
}

namespace UI::Overlay::System
{
	auto AffordancesUI::Draw(const Tick& tick, std::uint32_t handle) -> void
	{
		const Interaction interaction = tick.Interaction ?
			*tick.Interaction : Interaction{};

		DrawEngineInteraction(interaction);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		if (!tick.Affordances)
		{
			ImGui::TextDisabled("No affordance data.");
			return;
		}

		for (const Affordance& item : *tick.Affordances)
		{
			if (item.Handle == handle)
			{
				DrawDetails(item, tick.ObjectTable.get());
				return;
			}
		}

		ImGui::TextDisabled("Selected object is not an affordance.");
	}
}