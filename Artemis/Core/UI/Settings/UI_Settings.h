#pragma once

#include "Core/UI/Launcher/UI_Tab.h"

#include "External/imgui/imgui.h"

#include <atomic>
#include <string>

class State_Render;
class State_Settings;
class System_Settings;
class System_Logs;

struct UI_Settings_Dependencies
{
	State_Render& State_Render;
	State_Settings& State_Settings;
	System_Settings& System_Settings;
	System_Logs& System_Logs;
};

class UI_Settings : public UI_Tab
{
public:
	UI_Settings(UI_Settings_Dependencies dependencies) :
		m_Deps(dependencies) {}
	~UI_Settings() = default;

	void Draw();

	void DrawPathField(const char* label, const std::string& path, float widthOffset = 10.0f);

private:
	UI_Settings_Dependencies m_Deps;

	void DrawUserPreferences();
	void DrawHotkeysTable();
	void DrawDataPersistence();
	void DrawSystemDirectories();

	void DrawHotkeyRow(const char* label, const char* keys, const char* tooltip);

	void DrawPersistencePopups();
	void DrawConfirmDisableAppData();
	void DrawDeleteAllAppData();

	ImGuiTableFlags m_TableFlags = ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_NoBordersInBody;

	std::string m_AnimatePathLabel = "";
	float m_AnimationStartTime = 0.0f;
	const float m_AnimationDuration = 0.6f;

	float m_UIScalePreview = 1.0f;
	bool m_IsInitialized = false;
};