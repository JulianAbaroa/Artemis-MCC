#include "pch.h"
#include "Core/States/Core_State.h"
#include "Core/States/Domain/Core_State_Domain.h"
#include "Core/States/Domain/Tables/State_ObjectTable.h"
#include "Core/UI/Domain/Tables/UI_ObjectTable.h"
#include "External/imgui/imgui.h"
#include <string>
#include <vector>
#include <map>

void UI_ObjectTable::Draw()
{
	std::map<std::string, std::vector<LiveObject>> groupedObjects;

	bool hasMapChanged = g_pState->Domain->ObjectTable->HasChanged();
	if (hasMapChanged)
	{
		m_CacheObjects = g_pState->Domain->ObjectTable->GetObjectTable();
	}

	for (const auto& [handle, object] : m_CacheObjects)
	{
		groupedObjects[object.Class].push_back(object);
	}

	ImGui::TextDisabled("Live Objects Count: %d", m_CacheObjects.size());
	ImGui::Separator();

	if (ImGui::BeginChild("ObjectsRegion"))
	{
		float windowVisiblex2 = ImGui::GetWindowPos().x +
			ImGui::GetWindowContentRegionMax().x;

		for (auto& [className, list] : groupedObjects)
		{
			std::string headerLabel = 
				className + " (" + std::to_string(list.size()) + 
				")###header " + className;

			if (ImGui::CollapsingHeader(headerLabel.c_str()))
			{
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));

				for (size_t n = 0; n < list.size(); n++)
				{
					this->DrawObjectCard(list[n]);

					float lastCardRightEdge = ImGui::GetItemRectMax().x;
					float nextCardWidth = 300.0f;

					if (n + 1 < list.size()) 
					{
						float nextTagWidth = ImGui::CalcTextSize(("Tag: " + 
							list[n + 1].TagName).c_str()).x;

						nextCardWidth = (std::max)(250.0f, nextTagWidth + 25.0f);
					}

					float nextCardRightEdge = 
						lastCardRightEdge + 8.0f + nextCardWidth;

					if (n + 1 < list.size() && 
						nextCardRightEdge < windowVisiblex2)
					{
						ImGui::SameLine();
					}
				}

				ImGui::PopStyleVar();
				ImGui::Spacing();
			}
		}

		ImGui::EndChild();
	}
}


void UI_ObjectTable::Cleanup()
{
	m_CacheObjects.clear();
}


void UI_ObjectTable::DrawObjectCard(const LiveObject& object)
{
	std::string tagFullText = "Tag: " + object.TagName;
	float textWidth = ImGui::CalcTextSize(tagFullText.c_str()).x;

	float dynamicWidth = (std::max)(250.0f, textWidth + 25.0f);
	ImVec2 cardSize = ImVec2(dynamicWidth, 430.0f);

	ImGui::PushID(object.Handle);

	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f);

	if (ImGui::BeginChild(object.Handle, cardSize, true,
		ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
		ImGui::TextUnformatted("Tag:");
		ImGui::SameLine();

		ImGui::TextUnformatted(object.TagName.c_str());
		ImGui::PopStyleColor();

		ImGui::Separator();

		ImGui::Indent(5.0f);
		ImGui::TextDisabled("Class: %s", object.Class.c_str());
		ImGui::Text("Datum Index: 0x%08X", object.DatumIndex);
		ImGui::Text("Handle: 0x%08X", object.Handle);
		ImGui::Text("Sibling Handle: 0x%08X", object.NextSiblingHandle);
		ImGui::Text("Child Handle: 0x%08X", object.ChildHandle);
		ImGui::Text("Parent Handle: 0x%08X", object.ParentHandle);
		ImGui::Text("Address: 0x%012llX", object.Address);
		ImGui::Unindent(5.0f);

		ImGui::Spacing();
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "Kinematics");
		ImGui::Separator();

		ImGui::Indent(5.0f);

		ImGui::Text("Position: %.2f, %.2f, %.2f", object.Position[0], 
			object.Position[1], object.Position[2]);

		ImGui::Text("Forward: %.2f, %.2f, %.2f", object.Forward[0], 
			object.Forward[1], object.Forward[2]);

		ImGui::Text("Linear Vel: %.2f, %.2f, %.2f", object.LinearVelocity[0], 
			object.LinearVelocity[1], object.LinearVelocity[2]);

		ImGui::Text("Angular Vel: %.2f, %.2f, %.2f", object.AngularVelocity[0], 
			object.AngularVelocity[1], object.AngularVelocity[2]);

		ImGui::Unindent(5.0f);
	}

	ImGui::EndChild();
	ImGui::PopStyleVar();
	ImGui::PopID();
}