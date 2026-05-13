#include "pch.h"

// Header.
#include "Core_System.h"

// Domain.
#include "Domain/Core_System_Domain.h"

// Infrastructure.
#include "Infrastructure/Core_System_Infrastructure.h"

// Interface.
#include "Interface/System_Debug.h"

Core_System::Core_System()
{
	// Domain.
	Domain = std::make_unique<Core_System_Domain>();

	// Infrastructure.
	Infrastructure = std::make_unique<Core_System_Infrastructure>();

	// Interface.
	Debug = std::make_unique<System_Debug>();
}

Core_System::~Core_System() = default;