#include "pch.h"

#include "UI_ObjectTable.h"

#include "Core/States/Tables/Object/State_ObjectTable.h"

#include "Core/UI/Utils/String/EnumToString.h"
#include "Core/UI/Utils/Hex/HexFormater.h"

#include "External/imgui/imgui.h"

#include <format>

void UI_ObjectTable::Draw()
{
	if (!m_Visible) return;

	this->FirstDraw();
	this->ApplyResetIfRequested("Object Table");

	ImGuiWindowFlags flags = ImGuiWindowFlags_None;
	if (UI_Tab::s_Locked) flags |= ImGuiWindowFlags_NoMove;

	ImGui::Begin("Object Table", &m_Visible, flags);

	if (State_ObjectTable.HasChanged())
	{
		m_CacheObjects = State_ObjectTable.GetObjectTable();
		m_GroupedObjects.clear();

		for (const auto& [handle, object] : m_CacheObjects)
		{
			m_GroupedObjects[object.FourCC].push_back(
				&m_CacheObjects.at(handle));
		}

		State_ObjectTable.SetChanged(false);
	}

	ImGui::TextDisabled("Live Objects Count: %d", 
		m_CacheObjects.size());
	
	ImGui::Separator();

	m_SearchFilter.DrawSearchBar();

	ImGui::Separator();

	if (!ImGui::BeginChild("Objects Region"))
	{
		ImGui::EndChild();
		ImGui::End();
		return;
	}

	float windowRightEdge = ImGui::GetWindowPos().x +
		ImGui::GetWindowContentRegionMax().x;
	
	for (auto& [className, list] : m_GroupedObjects)
	{
		std::vector<const LiveObject*> filtered;
		for (const LiveObject* object : list)
		{
			if (m_SearchFilter.PassesFilter(*object))
			{
				filtered.push_back(object);
			}
		}

		if (filtered.empty()) continue;

		std::string headerLabel = className + " (" + 
			std::to_string(filtered.size()) +
			")###header " + className;

		if (!ImGui::CollapsingHeader(headerLabel.c_str()))
		{
			continue;
		}
		
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, 
			ImVec2(8, 8));

		for (size_t i = 0; i < filtered.size(); i++)
		{
			this->DrawObjectCard(*filtered[i]);

			float lastCardRightEdge = ImGui::GetItemRectMax().x;
			float nextCardWidth = 300.0f;

			if (i + 1 < filtered.size())
			{
				float nextTagWidth = ImGui::CalcTextSize(("Tag: " + 
					filtered[i + 1]->TagName).c_str()).x;
				
				nextCardWidth = (std::max)(250.0f, 
					nextTagWidth + 25.0f);
			}

			float nextCardRightEdge = lastCardRightEdge + 
				8.0f + nextCardWidth;

			if (i + 1 < filtered.size() &&
				nextCardRightEdge < windowRightEdge)
			{
				ImGui::SameLine();
			}
		}

		ImGui::PopStyleVar();
		ImGui::Spacing();
	}

	ImGui::EndChild();

	ImGui::End();
}

void UI_ObjectTable::DrawObjectCard(const LiveObject& object)
{
	float lineHeight = ImGui::GetTextLineHeightWithSpacing();

	std::string tagFullText = "Tag: " + object.TagName;
	float dynamicWidth = (std::max)(250.0f, ImGui::CalcTextSize(tagFullText.c_str()).x + 25.0f);

	ImVec2 cardSize = ImVec2(dynamicWidth, 270.0f);

	ImGui::PushID(object.Handle);
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f);

	auto flags = ImGuiWindowFlags_NoScrollbar | 
		ImGuiWindowFlags_NoScrollWithMouse;

	if (ImGui::BeginChild(object.Handle, cardSize, true, flags))
	{
		this->DrawCardHeader(object);
		this->DrawCardFields(object);
	}

	ImGui::EndChild();
	ImGui::PopStyleVar();
	ImGui::PopID();
}

void UI_ObjectTable::DrawCardHeader(const LiveObject& object)
{
	ImGui::PushStyleColor(ImGuiCol_Text, 
		ImVec4(0.4f, 0.8f, 1.0f, 1.0f));

	ImGui::TextUnformatted("Tag:");

	ImGui::SameLine();

	ImGui::TextUnformatted(object.TagName.c_str());

	ImGui::PopStyleColor();

	ImGui::Separator();
}

void UI_ObjectTable::DrawCardFields(const LiveObject& object)
{
	ImGui::Indent(5.0f);

	m_CopyableField.Draw("Class:", object.FourCC, object.Handle);

	m_CopyableField.Draw("Datum Index:", HexFormater::Hex32(
		object.DatumIndex), object.Handle);

	m_CopyableField.Draw("Handle:",	HexFormater::Hex32(object.Handle), 
		object.Handle);

	m_CopyableField.Draw("Sibling Handle:", HexFormater::Hex32(
		object.NextSiblingHandle), object.Handle);

	m_CopyableField.Draw("Child Handle:", HexFormater::Hex32(
		object.ChildHandle), object.Handle);

	m_CopyableField.Draw("Parent Handle:", HexFormater::Hex32(
		object.ParentHandle), object.Handle);

	m_CopyableField.Draw("Address:", HexFormater::Hex64(object.Address), 
		object.Handle);

	ImGui::Unindent(5.0f);
}

void UI_ObjectTable::Cleanup()
{
	m_CacheObjects.clear();
	m_GroupedObjects.clear();
}