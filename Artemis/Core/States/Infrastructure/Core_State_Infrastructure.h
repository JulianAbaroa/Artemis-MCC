#pragma once

#include <memory>

// --- Engine ---

// Input.
class State_Input;

// Memory.
class State_MemoryScanner;

// Lifecycle.
class State_Lifecycle;

// Render.
class State_Render;

// --- Persistence ---

// Settings.
class State_Settings;

// Main container for the application's infrastructure states.
struct Core_State_Infrastructure
{
	Core_State_Infrastructure();
	~Core_State_Infrastructure();

	// Engine.
	std::unique_ptr<State_Input> Input;
	std::unique_ptr<State_MemoryScanner> MemoryScanner;
	std::unique_ptr<State_Lifecycle> Lifecycle;
	std::unique_ptr<State_Render> Render;

	// Persistence.
	std::unique_ptr<State_Settings> Settings;
};