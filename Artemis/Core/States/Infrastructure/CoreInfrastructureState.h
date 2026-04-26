#pragma once

#include <memory>

// Engine
class InputState;
class LifecycleState;
class RenderState;

// Persistence
class SettingsState;

// Main container for the application's infrastructure states.
struct CoreInfrastructureState
{
	CoreInfrastructureState();
	~CoreInfrastructureState();

	std::unique_ptr<InputState> Input;
	std::unique_ptr<LifecycleState> Lifecycle;
	std::unique_ptr<RenderState> Render;
	std::unique_ptr<SettingsState> Settings;
};