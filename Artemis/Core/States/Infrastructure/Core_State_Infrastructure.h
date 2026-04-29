#pragma once

#include <memory>

// Engine
class State_Input;
class State_Lifecycle;
class State_Render;

// Persistence
class State_Settings;

// Main container for the application's infrastructure states.
struct Core_State_Infrastructure
{
	Core_State_Infrastructure();
	~Core_State_Infrastructure();

	std::unique_ptr<State_Input> Input;
	std::unique_ptr<State_Lifecycle> Lifecycle;
	std::unique_ptr<State_Render> Render;
	std::unique_ptr<State_Settings> Settings;
};