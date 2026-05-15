#pragma once

#include <string>

class State_Lifecycle;
class State_Settings;
class State_Render;
class System_Debug;

struct System_Preferences_Dependencies
{
	State_Lifecycle& State_Lifecycle;
	State_Settings& State_Settings;
	State_Render& State_Render;
	System_Debug& System_Debug;
};

class System_Preferences
{
public:
	System_Preferences(System_Preferences_Dependencies dependencies) :
		m_Deps(dependencies) {};
	~System_Preferences() = default;

	void SavePreferences();
	void LoadPreferences();

private:
	System_Preferences_Dependencies m_Deps;

	std::string GetPreferencesFilePath() const;
	void ParseLine(const std::string& line);

	//void SaveFFmpegState(std::ofstream& file);
	void SaveLifeCycleState(std::ofstream& file);
	void SaveSettingsState(std::ofstream& file);
	void SaveUI(std::ofstream& file);

	//void LoadFFmpegState(std::string& key, std::string& value);
	void LoadLifecycleState(std::string& key, std::string& value);
	void LoadSettingsState(std::string& key, std::string& value);
	void LoadUI(std::string& key, std::string& value);
};