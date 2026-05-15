#include "pch.h"

// Header.
#include "Core_System_Infrastructure.h"

#include "Core/States/Core_State.h"
#include "Core/States/Infrastructure/Core_State_Infrastructure.h"

#include "Core/Systems/Core_System.h"
#include "Core/Systems/Interface/Core_System_Interface.h"

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

Core_System_Infrastructure::Core_System_Infrastructure() = default;
Core_System_Infrastructure::~Core_System_Infrastructure() = default;

void Core_System_Infrastructure::Initialize(Core_State& state, 
	Core_System_Interface& systemInterface)
{
	// Input.
	Input = std::make_unique<System_Input>(
		System_Input_Dependencies{
			.State_Input = *state.Infrastructure->Input,
			.System_Debug = *systemInterface.Debug,
		}
	);

	// MemoryReader.
	MemoryReader = std::make_unique<System_MemoryReader>();

	// MemoryScanner.
	MemoryScanner = std::make_unique<System_MemoryScanner>(
		System_MemoryScanner_Dependencies{
			.State_MemoryScanner = *state.Infrastructure->MemoryScanner,
			.System_Debug = *systemInterface.Debug,
		}
	);

	// AOBScanner
	AOBScanner = std::make_unique<System_AOBScanner>(
		*systemInterface.Debug);

	// Lifecycle.
	Lifecycle = std::make_unique<System_Lifecycle>(
		System_Lifecycle_Dependencies {
			.State_Lifecycle = *state.Infrastructure->Lifecycle,
			.System_Debug = *systemInterface.Debug,
		}
	);

	// Render.
	Render = std::make_unique<System_Render>(
		System_Render_Dependencies {
			.State_Render = *state.Infrastructure->Render,
			.State_Settings = *state.Infrastructure->Settings,
			.System_Debug = *systemInterface.Debug,
		}
	);

	// Preferences.
	Preferences = std::make_unique<System_Preferences>(
		System_Preferences_Dependencies {
			.State_Lifecycle = *state.Infrastructure->Lifecycle,
			.State_Settings = *state.Infrastructure->Settings,
			.State_Render = *state.Infrastructure->Render,
			.System_Debug = *systemInterface.Debug,
		}
	);

	// Settings.
	Settings = std::make_unique<System_Settings>(
		System_Settings_Dependencies {
			.State_Settings = *state.Infrastructure->Settings,
			.System_Debug = *systemInterface.Debug,
		}
	);

	// Thread.
	Thread = std::make_unique<System_Thread>(
		*state.Infrastructure->Lifecycle);
}

void Core_System_Infrastructure::Shutdown()
{
	Input.reset();
	MemoryReader.reset();
	MemoryScanner.reset();
	AOBScanner.reset();
	Lifecycle.reset();
	Render.reset();
	Preferences.reset();
	Settings.reset();
	Thread.reset();
}