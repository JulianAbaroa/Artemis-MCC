#include "pch.h"
#include "Core/UI/Tabs/Domain/Map/MapTab.h"
#include "Core/UI/Tabs/Domain/Map/Phmo/PhmoViewer.h"

MapTab::MapTab()
{
	m_Viewers["phmo"] = std::make_unique<PhmoViewer>();
}

void MapTab::Draw()
{
	this->DrawLeftPanel();
	ImGui::SameLine();
	this->DrawRightPanel();
}

void MapTab::DrawLeftPanel()
{
	ImGui::BeginChild("LeftPanel", ImVec2(ImGui::GetWindowWidth() * 0.3f, 0), true);

	m_SearchFilter.Draw("##search tags...", -1.0f);
	ImGui::Separator();

	for (const auto& [className, viewer] : m_Viewers)
	{
		std::string treeLabel = className + " - " + viewer->GetShortDescription();

		if (ImGui::TreeNode(treeLabel.c_str()))
		{
			auto tags = viewer->GetAvailableTags();
			for (const auto& tagName : tags)
			{
				if (!m_SearchFilter.PassFilter(tagName.c_str())) continue;

				bool isSelected = (m_SelectedTag == tagName);
				if (ImGui::Selectable(tagName.c_str(), isSelected))
				{
					m_SelectedTag = tagName;
					m_SelectedClass = className;
				}
			}

			ImGui::TreePop();
		}
	}

	ImGui::EndChild();
}

void MapTab::DrawRightPanel()
{
	ImGui::BeginChild("RightPanel", ImVec2(0, 0), true);

	if (!m_SelectedClass.empty() && !m_SelectedTag.empty())
	{
		auto it = m_Viewers.find(m_SelectedClass);
		if (it != m_Viewers.end())
		{
			it->second->DrawData(m_SelectedTag);
		}
		else
		{
			ImGui::Text("No viewer implemented yet for class: %s",
				m_SelectedClass.c_str());
		}
	}
	else
	{
		ImGui::TextDisabled("Select a tag from the left panel"
			" to inspect its static data.");
	}

	ImGui::EndChild();
}