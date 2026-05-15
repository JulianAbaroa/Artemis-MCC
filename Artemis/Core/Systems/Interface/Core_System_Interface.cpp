#include "pch.h"

// Header.
#include "Core_System_Interface.h"

#include "Core/States/Core_State.h"
#include "Core/States/Infrastructure/Core_State_Infrastructure.h"
#include "Core/States/Interface/Core_State_Interface.h"

// Debug.
#include "Debug/System_Debug.h"

Core_System_Interface::Core_System_Interface() = default;
Core_System_Interface::~Core_System_Interface() = default;

void Core_System_Interface::Initialize(Core_State& state)
{
	Debug = std::make_unique<System_Debug>(
		System_Debug_Dependencies {
			.State_Settings = *state.Infrastructure->Settings,
			.State_Debug = *state.Interface->Debug,
		}
	);
}

void Core_System_Interface::Shutdown()
{
	Debug.reset();
}