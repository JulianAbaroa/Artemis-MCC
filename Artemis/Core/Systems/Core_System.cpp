#include "pch.h"

// Header.
#include "Core_System.h"

// State.
#include "Core/States/Core_State.h"

#include "Domain/Core_System_Domain.h"
#include "Infrastructure/Core_System_Infrastructure.h"
#include "Interface/Core_System_Interface.h"

Core_System::Core_System() = default;
Core_System::~Core_System() = default;

void Core_System::Initialize(Core_State& state)
{
	// Interface.
	Interface = std::make_unique<Core_System_Interface>();
	Interface->Initialize(state);

	// Infrastructure.
	Infrastructure = std::make_unique<Core_System_Infrastructure>();
	Infrastructure->Initialize(state, *Interface);

	// Domain.
	Domain = std::make_unique<Core_System_Domain>();
	Domain->Initialize(state, *Infrastructure, *Interface);
}

void Core_System::Shutdown() const
{
	Interface->Shutdown();
	Infrastructure->Shutdown();
	Domain->Shutdown();
}