#pragma once

#include <memory>

// Domain.
struct Core_System_Domain;

// Infrastructure.
struct Core_System_Infrastructure;

// Interface.
class System_Debug;

// Main container for the application's systems.
struct Core_System
{
	Core_System();
	~Core_System();

	// Domain.
	std::unique_ptr<Core_System_Domain> Domain;

	// Infrastructure.
	std::unique_ptr<Core_System_Infrastructure> Infrastructure;

	// Interface.
	std::unique_ptr<System_Debug> Debug;
};

extern Core_System* g_pSystem;