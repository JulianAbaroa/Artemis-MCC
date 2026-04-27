#pragma once

#include "Core/UI/Tabs/Domain/Map/ITagViewer.h"
#include "External/imgui/imgui.h"
#include <unordered_map>
#include <memory>
#include <string>

class MapTab
{
public:
	MapTab();
	void Draw();

private:
	std::unordered_map < std::string, std::unique_ptr<ITagViewer>> m_Viewers;

	std::string m_SelectedClass;
	std::string m_SelectedTag;

	ImGuiTextFilter m_SearchFilter;

	void DrawLeftPanel();
	void DrawRightPanel();
};