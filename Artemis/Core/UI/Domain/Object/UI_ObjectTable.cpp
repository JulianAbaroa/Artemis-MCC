#include "pch.h"

// Header.
#include "UI_ObjectTable.h"

// --- States ---

#include "Core/States/Domain/Object/State_ObjectTable.h"

// --- UI ---

#include "Core/UI/Utils/Strings/EnumToString.h"

// ImGui.
#include "External/imgui/imgui.h"

#include <variant>
#include <string>
#include <vector>
#include <map>

void UI_ObjectTable::Draw()
{
	// Only update if the object table has changed.
	if (Sta_ObjectTable.HasChanged())
	{
		m_CacheObjects = Sta_ObjectTable.GetObjectTable();
		m_GroupedObjects.clear();

		for (const auto& [handle, object] : m_CacheObjects)
		{
			m_GroupedObjects[object.FourCC].push_back(&m_CacheObjects.at(handle));
		}
	}

	ImGui::TextDisabled("Live Objects Count: %d", m_CacheObjects.size());
	ImGui::Separator();

	m_SearchFilter.DrawSearchBar();
	ImGui::Separator();

	if (!ImGui::BeginChild("ObjectsRegion"))
	{
		ImGui::EndChild();
		return;
	}

	float windowRightEdge = ImGui::GetWindowPos().x +
		ImGui::GetWindowContentRegionMax().x;
	
	for (auto& [className, list] : m_GroupedObjects)
	{
		std::vector<const LiveObject*> filtered;
		for (const LiveObject* obj : list)
		{
			if (m_SearchFilter.PassesFilter(*obj))
				filtered.push_back(obj);
		}

		if (filtered.empty()) continue;

		std::string headerLabel = className + " (" + std::to_string(filtered.size()) +
			")###header " + className;

		if (ImGui::CollapsingHeader(headerLabel.c_str()))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));

			for (size_t i = 0; i < filtered.size(); i++)
			{
				DrawObjectCard(*filtered[i]);

				float lastCardRightEdge = ImGui::GetItemRectMax().x;
				float nextCardWidth = 300.0f;

				if (i + 1 < filtered.size())
				{
					float nextTagWidth = ImGui::CalcTextSize(
						("Tag: " + filtered[i + 1]->TagName).c_str()).x;
					nextCardWidth = (std::max)(250.0f, nextTagWidth + 25.0f);
				}

				float nextCardRightEdge = lastCardRightEdge + 8.0f + nextCardWidth;

				if (i + 1 < filtered.size() && nextCardRightEdge < windowRightEdge)
					ImGui::SameLine();
			}

			ImGui::PopStyleVar();
			ImGui::Spacing();
		}
	}

	ImGui::EndChild();
}


void UI_ObjectTable::Cleanup()
{
	m_CacheObjects.clear();
	m_GroupedObjects.clear();
}

// --- Card-related ---

void UI_ObjectTable::DrawObjectCard(const LiveObject& object)
{
	float lineHeight = ImGui::GetTextLineHeightWithSpacing();
	float specificHeight = this->GetSpecificHeight(object, lineHeight);

	std::string tagFullText = "Tag: " + object.TagName;
	float dynamicWidth = (std::max)(250.0f, ImGui::CalcTextSize(tagFullText.c_str()).x + 25.0f);

	ImVec2 cardSize = ImVec2(dynamicWidth, 520.0f + specificHeight);

	ImGui::PushID(object.Handle);
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f);

	if (ImGui::BeginChild(object.Handle, cardSize, true,
		ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
	{
		this->DrawCardHeader(object);
		this->DrawCardBaseFields(object);
		this->DrawCardKinematics(object);
		this->DrawTypeSpecificSection(object);
	}

	ImGui::EndChild();
	ImGui::PopStyleVar();
	ImGui::PopID();
}

void UI_ObjectTable::DrawCardHeader(const LiveObject& object)
{
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
	ImGui::TextUnformatted("Tag:");
	ImGui::SameLine();
	ImGui::TextUnformatted(object.TagName.c_str());
	ImGui::PopStyleColor();
	ImGui::Separator();
}

void UI_ObjectTable::DrawCardBaseFields(const LiveObject& object)
{
	char buf[64];

	ImGui::Indent(5.0f);
	m_CopyableField.Draw("Class:", object.FourCC, object.Handle);

	snprintf(buf, sizeof(buf), "0x%08X", object.DatumIndex);		
	m_CopyableField.Draw("Datum Index:", buf, object.Handle);

	snprintf(buf, sizeof(buf), "0x%08X", object.Handle);			
	m_CopyableField.Draw("Handle:", buf, object.Handle);

	snprintf(buf, sizeof(buf), "0x%08X", object.NextSiblingHandle); 
	m_CopyableField.Draw("Sibling Handle:", buf, object.Handle);

	snprintf(buf, sizeof(buf), "0x%08X", object.ChildHandle);		
	m_CopyableField.Draw("Child Handle:", buf, object.Handle);

	snprintf(buf, sizeof(buf), "0x%08X", object.ParentHandle);		
	m_CopyableField.Draw("Parent Handle:", buf, object.Handle);

	snprintf(buf, sizeof(buf), "0x%012llX", object.Address);		
	m_CopyableField.Draw("Address:", buf, object.Handle);
	
}

void UI_ObjectTable::DrawCardKinematics(const LiveObject& object)
{
	ImGui::Spacing();
	ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "Kinematics");
	ImGui::Separator();

	ImGui::Indent(5.0f);
	ImGui::Text("Position: %.2f, %.2f, %.2f", object.Position[0], object.Position[1], object.Position[2]);
	ImGui::Text("Forward: %.2f, %.2f, %.2f", object.Forward[0], object.Forward[1], object.Forward[2]);
	ImGui::Text("Up: %.2f, %.2f, %.2f", object.Up[0], object.Up[1], object.Up[2]);
	ImGui::Text("Radius: %.2f", object.CurrentRadius);
	ImGui::Text("Linear Vel: %.2f, %.2f, %.2f", object.LinearVelocity[0], object.LinearVelocity[1], object.LinearVelocity[2]);
	ImGui::Text("Angular Vel: %.2f, %.2f, %.2f", object.AngularVelocity[0], object.AngularVelocity[1], object.AngularVelocity[2]);
	ImGui::Unindent(5.0f);
}

// --- Type-related ---

void UI_ObjectTable::DrawSectionBiped(const BipedObject& specific)
{
	ImGui::Spacing();
	ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Biped");
	ImGui::Separator();

	ImGui::Indent(5.0f);
	ImGui::Text("Is Crouched: %s", specific.IsCrouched ? "Yes" : "No");
	ImGui::Text("Is Ability Active: %s", specific.IsAbilityActive ? "Yes" : "No");
	ImGui::Text("Is Grounded: %s", specific.IsGrounded ? "Yes" : "No");
	ImGui::Text("Movement Direction: %.2f (X), %.2f (Y)", specific.MovementDirection[0], specific.MovementDirection[1]);
	ImGui::Text("Health: %.2f", specific.Health);
	ImGui::Text("Regeneration Delay: %u", specific.HealthRegenerationDelay);
	ImGui::Text("Shields: %.2f", specific.Shields);
	ImGui::Text("Has Shields: %s", specific.HasNoShields ? "No" : "Yes");
	ImGui::Text("Zoom Level: %s", EnumToString::ZoomLevelToString(specific.ZoomLevel));
	ImGui::Unindent(5.0f);
}

void UI_ObjectTable::DrawSectionVehicle(const VehicleObject& specific)
{
	ImGui::Spacing();
	ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Vehicle");
	ImGui::Separator();

	ImGui::Indent(5.0f);
	ImGui::Text("Type: %s", EnumToString::VehicleTypeToString(specific.Type));
	ImGui::Text("Health: %.2f", specific.Health);
	ImGui::Text("Regen Delay: %u", specific.RegenerationDelay);

	if (specific.SeatLayout != nullptr)
	{
		ImGui::Spacing();
		ImGui::Text("Seats:");
		ImGui::Indent(5.0f);
		for (const auto& seat : specific.SeatLayout->seats)
		{
			bool free = specific.IsSeatFree(seat);

			ImVec4 color = free ? 
				ImVec4(0.4f, 1.0f, 0.4f, 1.0f) : ImVec4(1.0f, 0.4f, 0.4f, 1.0f);

			if (!seat.IsHijackable && free)
				color = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);

			ImGui::TextColored(color, "[%s]%s %s",
				free ? "FREE" : "OCCUPIED",
				seat.IsHijackable ? " (hijack)" : "",
				seat.Name);
		}
		ImGui::Unindent(5.0f);
	}

	ImGui::Unindent(5.0f);
}

void UI_ObjectTable::DrawSectionWeapon(const WeaponObject& specific)
{
	ImGui::Spacing();
	ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Weapon");
	ImGui::Separator();

	ImGui::Indent(5.0f);
	ImGui::Text("Total Ammo: %u", specific.TotalAmmo);
	ImGui::Text("Current Ammo: %u", specific.CurrentAmmo);
	ImGui::Text("Is Reloading: %s", specific.IsReloading ? "Yes" : "No");
	ImGui::Text("Total Heat: %.2f", specific.TotalHeat);
	ImGui::Text("Total Energy: %.2f", specific.TotalEnergy);
	ImGui::Text("Charge Progress: %u", specific.ChargeProgress);
	ImGui::Text("Pressing Fire: %s", specific.IsFiring ? "Yes" : "No");
	ImGui::Text("Action State: %s", EnumToString::ActionStateToString(specific.ActionState));
	ImGui::Unindent(5.0f);
}

void UI_ObjectTable::DrawSectionEquipment(const EquipmentObject& specific)
{
	ImGui::Spacing();
	ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Equipment");
	ImGui::Separator();

	ImGui::Indent(5.0f);
	ImGui::Text("Energy: %.2f", specific.TotalEnergy);
	ImGui::Unindent(5.0f);
}

void UI_ObjectTable::DrawShapeFields(const ZoneShape& shape)
{
	ImGui::Text("Shape: %s", EnumToString::ShapeTypeToString(shape.ShapeType));
	ImGui::Text("Radius: %.3f", shape.Radius);
	ImGui::Text("Length: %.3f", shape.Length);
	ImGui::Text("Top: %.3f", shape.Top);
	ImGui::Text("Bottom: %.3f", shape.Bottom);
}

void UI_ObjectTable::DrawSectionCrate(const CrateObject& specific)
{
	ImGui::Spacing();
	ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Crate");
	ImGui::Separator();

	ImGui::Indent(5.0f);
	ImGui::Text("Type: %s", EnumToString::CrateTypeToString(specific.Type));

	if (specific.Zone.has_value())
	{
		const auto& z = *specific.Zone;

		ImGui::Spacing();

		ImGui::Text("[ Zone ]");
		ImGui::Indent(5.0f);
		ImGui::Text("Team: %s", EnumToString::TeamToString(z.Team));
		this->DrawShapeFields(z.Shape);
		ImGui::Unindent(5.0f);
	}
	else if (specific.Teleporter.has_value())
	{
		const auto& t = *specific.Teleporter;

		ImGui::Spacing();

		ImGui::Text("[ Teleporter ]");
		ImGui::Indent(5.0f);
		ImGui::Text("Channel: %u", t.Channel);
		ImGui::Text("Allowed:");

		ImGui::Indent(5.0f);
		this->DrawTeleporterAllowedObjects(t.AllowedObjects);
		ImGui::Unindent(5.0f);
		this->DrawShapeFields(t.ZoneShape);
		ImGui::Unindent(5.0f);
	}
	else if (specific.Destructible.has_value())
	{
		const auto& d = *specific.Destructible;

		ImGui::Spacing();

		ImGui::Text("[ Destructible ]");

		ImGui::Indent(5.0f);
		ImGui::Text("Health: %.2f", d.Health);
		if (d.RegenerationDelay.has_value())
			ImGui::Text("Regen Delay: %u", *d.RegenerationDelay);
		ImGui::Unindent(5.0f);
	}

	ImGui::Unindent(5.0f);
}

void UI_ObjectTable::DrawSectionScenery(const SceneryObject& specific)
{
	ImGui::Spacing();
	ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Scenery");
	ImGui::Separator();

	ImGui::Indent(5.0f);
	ImGui::Text("Type: %s", EnumToString::SceneryTypeToString(specific.Type));

	if (specific.Spawn.has_value())
	{
		ImGui::Spacing();
		ImGui::Text("[ Spawn Point ]");
		ImGui::Indent(5.0f);
		ImGui::Text("Team: %s", EnumToString::TeamToString(specific.Spawn->Team));
		ImGui::Unindent(5.0f);
	}
	else if (specific.Boundary.has_value())
	{
		const auto& b = *specific.Boundary;
		ImGui::Spacing();
		ImGui::Text("[ Boundary ]");
		ImGui::Indent(5.0f);
		ImGui::Text("Team: %s", EnumToString::TeamToString(b.Team));
		DrawShapeFields(b.Shape);
		ImGui::Unindent(5.0f);
	}

	ImGui::Unindent(5.0f);
}

void UI_ObjectTable::DrawTypeSpecificSection(const LiveObject& object)
{
	std::visit([&](auto&& specific) {
		using T = std::decay_t<decltype(specific)>;

		if constexpr (std::is_same_v<T, BipedObject>)
			this->DrawSectionBiped(specific);

		else if constexpr (std::is_same_v<T, VehicleObject>)
			this->DrawSectionVehicle(specific);

		else if constexpr (std::is_same_v<T, WeaponObject>)
			this->DrawSectionWeapon(specific);

		else if constexpr (std::is_same_v<T, EquipmentObject>)
			this->DrawSectionEquipment(specific);

		else if constexpr (std::is_same_v<T, CrateObject>)
			this->DrawSectionCrate(specific);

		else if constexpr (std::is_same_v<T, SceneryObject>)	
			this->DrawSectionScenery(specific);
		}, object.SpecificObject);
}

// --- Helpers ---

float UI_ObjectTable::GetCrateSpecificHeight(const CrateObject& specific, float lineHeight)
{
	float h = lineHeight;

	if (specific.Zone.has_value())
		h += lineHeight * 7;

	else if (specific.Teleporter.has_value())
		h += lineHeight * 14;

	else if (specific.Destructible.has_value())
		h += lineHeight * 3;

	return h + 24.0f;
}

float UI_ObjectTable::GetScenerySpecificHeight(const SceneryObject& specific, float lineHeight)
{
	float h = lineHeight;

	if (specific.Spawn.has_value())
		h += lineHeight * 2;

	else if (specific.Boundary.has_value())
		h += lineHeight * 7;

	return h + 24.0f;
}

float UI_ObjectTable::GetSpecificHeight(const LiveObject& object, float lineHeight)
{
	return std::visit([&](auto&& specific) -> float {
		using T = std::decay_t<decltype(specific)>;

		if constexpr (std::is_same_v<T, BipedObject>) 
			return lineHeight * 11 + 24.0f;

		if constexpr (std::is_same_v<T, WeaponObject>) 
			return lineHeight * 9 + 24.0f;

		if constexpr (std::is_same_v<T, EquipmentObject>) 
			return lineHeight * 1 + 24.0f;

		if constexpr (std::is_same_v<T, VehicleObject>)
		{
			size_t seatCount = specific.SeatLayout ? specific.SeatLayout->seats.size() : 0;
			return lineHeight * (4 + seatCount) + 24.0f;
		}

		if constexpr (std::is_same_v<T, CrateObject>) 
			return this->GetCrateSpecificHeight(specific, lineHeight);

		if constexpr (std::is_same_v<T, SceneryObject>) 
			return this->GetScenerySpecificHeight(specific, lineHeight);

		return 0.0f;
		}, object.SpecificObject);
}

void UI_ObjectTable::DrawTeleporterAllowedObjects(AllowedObjects allowed)
{
	using T = AllowedObjects;
	uint8_t raw = static_cast<uint8_t>(allowed);

	struct Flag { uint8_t bit; const char* label; bool inverted; };
	constexpr Flag flags[] = {
		{ 0x01, "Players",          true  },
		{ 0x02, "Ground Vehicles",  false },
		{ 0x04, "Heavy Vehicles",   false },
		{ 0x08, "Flying Vehicles",  false },
		{ 0x10, "Projectiles",      false },
	};

	for (const auto& f : flags)
	{
		bool bitSet = (raw & f.bit) != 0;
		bool active = f.inverted ? !bitSet : bitSet;

		ImVec4 color = active ? 
			ImVec4(0.4f, 1.0f, 0.4f, 1.0f) : ImVec4(1.0f, 0.4f, 0.4f, 1.0f);

		ImGui::TextColored(color, "%s %s", active ? "[+]" : "[-]", f.label);
	}
}