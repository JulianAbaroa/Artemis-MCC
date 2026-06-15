#pragma once

#include "Core/UI/Launcher/UI_Tab.h"

#include <atomic>
#include <vector>
#include <string>

struct LogFilterState
{
	std::string SearchStr;
	bool IsFiltering;
};

struct LogEntry;

class State_Settings;
class State_Logs;
class System_Logs;

struct UI_Logs_Deps
{
	State_Settings& State_Settings;
	State_Logs& State_Logs;
	System_Logs& System_Logs;
};

class UI_Logs : public UI_Tab
{
public:
	UI_Logs(UI_Logs_Deps dependencies) : m_Deps(dependencies) {}
	~UI_Logs() = default;

	void Draw();
	
private:
	UI_Logs_Deps m_Deps;

	LogFilterState DrawTopBar();
	void DrawClearButton(bool isFiltering, std::string& searchStr);
	void DrawCopyButton(bool isFiltering, std::string& searchStr);
	void DrawHelpMarker();

	std::vector<int> GetFilteredIndices(const LogFilterState& filter);
	void DrawScrollingRegion(const LogFilterState& filter);
	void DrawLogLine(int realIndex, const LogEntry& entry, bool& logClickedThisFrame);
	void DrawLogMessage(const std::string& message);
	void HandleLogInteraction(int realIndex, const LogEntry& entry, bool& logClickedThisFrame);

	void DrawSearchBar(char* buffer, size_t bufferSize);
	bool IsIndexSelected(int index) const;

	char m_SearchBuffer[128] = "";

	std::atomic<int> m_SelectionEnd{ -1 };
	std::atomic<int> m_SelectionStart{ -1 };
	std::atomic<float> m_AnimationStartTime{ 0.0f };
	std::atomic<int> m_AnimateIndex{ -1 };
	const float m_AnimationDuration = 0.8f;
};