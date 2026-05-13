#include "pch.h"

// Header.
#include "Core_System_Infrastructure.h"

// Memory Reader.
#include "Engine/Memory/System_MemoryReader.h"

// Memory Scanner.
#include "Engine/Memory/System_MemoryScanner.h"

// AOB Scanner.
#include "Engine/Memory/System_AOBScanner.h"

// Input.
#include "Engine/Input/System_Input.h"

// Lifecycle.
#include "Engine/Lifecycle/System_Lifecycle.h"

// Render.
#include "Engine/Render/System_Render.h"

// Preferences.
#include "Persistence/System_Preferences.h"

// Settings.
#include "Persistence/System_Settings.h"

// Thread.
#include "Engine/Thread/System_Thread.h"

Core_System_Infrastructure::Core_System_Infrastructure()
{
	// Input.
	Input = std::make_unique<System_Input>();

	// MemoryReader.
	MemoryReader = std::make_unique<System_MemoryReader>();

	// MemoryScanner.
	MemoryScanner = std::make_unique<System_MemoryScanner>();

	// AOBScanner
	AOBScanner = std::make_unique<System_AOBScanner>();

	// Lifecycle.
	Lifecycle = std::make_unique<System_Lifecycle>();

	// Render.
	Render = std::make_unique<System_Render>();

	// Preferences.
	Preferences = std::make_unique<System_Preferences>();

	// Settings.
	Settings = std::make_unique<System_Settings>();

	// Thread.
	Thread = std::make_unique<System_Thread>();
}

Core_System_Infrastructure::~Core_System_Infrastructure() = default;