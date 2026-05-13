#include "pch.h"

// Header.
#include "Core_State_Infrastructure.h"

// Input.
#include "Engine/Input/State_Input.h"

// Memory.
#include "Engine/Memory/State_MemoryScanner.h"

// Lifecycle.
#include "Engine/Lifecycle/State_Lifecycle.h"

// Render.
#include "Engine/Render/State_Render.h"

// Settings.
#include "Persistence/State_Settings.h"

Core_State_Infrastructure::Core_State_Infrastructure()
{
	// Engine.
	Input = std::make_unique<State_Input>();
	MemoryScanner = std::make_unique<State_MemoryScanner>();
	Lifecycle = std::make_unique<State_Lifecycle>();
	Render = std::make_unique<State_Render>();

	// Persistence.
	Settings = std::make_unique<State_Settings>();
}

Core_State_Infrastructure::~Core_State_Infrastructure() = default;