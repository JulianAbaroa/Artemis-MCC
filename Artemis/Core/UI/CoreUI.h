#pragma once

#include <memory>

class MainInterface;
class SettingsTab;
class LogsTab;

class ObjectTableTab;
class PlayerTableTab;
class ObjectGraphTab;
class InteractableTab;
class PhysicsDebugTab;

// Main container for the application's UIs.
struct CoreUI
{
	CoreUI();
	~CoreUI();

	std::unique_ptr<MainInterface> Main;
	std::unique_ptr<SettingsTab> Settings;
	std::unique_ptr<LogsTab> Logs;

	std::unique_ptr<ObjectTableTab> ObjectTable;
	std::unique_ptr<PlayerTableTab> PlayerTable;
	std::unique_ptr<ObjectGraphTab> ObjectGraph;
	std::unique_ptr<InteractableTab> Interactable;
	std::unique_ptr<PhysicsDebugTab> PhysicsDebug;
};

extern CoreUI* g_pUI;