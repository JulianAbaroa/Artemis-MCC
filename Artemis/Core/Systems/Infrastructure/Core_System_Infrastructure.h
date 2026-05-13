#pragma once

#include <memory>

// --- Engine ---

// Input.
class System_Input;

// Memory Reader.
class System_MemoryReader;
	
// Memory Scanner.
class System_MemoryScanner;

// AOB Scanner.
class System_AOBScanner;

// Lifecycle.
class System_Lifecycle;

// Render.
class System_Render;

// Thread.
class System_Thread;

// --- Persistence ---

// Preferences.
class System_Preferences;

// Settings.
class System_Settings;

struct Core_System_Infrastructure
{
	Core_System_Infrastructure();
	~Core_System_Infrastructure();

	// --- Engine ---

	// Input.
	std::unique_ptr<System_Input> Input;

	// Memory Reader.
	std::unique_ptr<System_MemoryReader> MemoryReader;

	// Memory Scanner.
	std::unique_ptr<System_MemoryScanner> MemoryScanner;

	// AOB Scanner.
	std::unique_ptr<System_AOBScanner> AOBScanner;

	// Lifecycle.
	std::unique_ptr<System_Lifecycle> Lifecycle;

	// Render.
	std::unique_ptr<System_Render> Render;

	// Thread.
	std::unique_ptr<System_Thread> Thread;

	// --- Persistence ---

	// Preferences.
	std::unique_ptr<System_Preferences> Preferences;

	// Settings.
	std::unique_ptr<System_Settings> Settings;

};