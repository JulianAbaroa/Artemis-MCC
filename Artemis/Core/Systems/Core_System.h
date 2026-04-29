#pragma once

#include <memory>

struct Core_System_Domain;
struct Core_System_Infrastructure;
class System_Debug;

// Main container for the application's systems.
struct Core_System
{
	Core_System();
	~Core_System();

	std::unique_ptr<Core_System_Domain> Domain;
	std::unique_ptr<Core_System_Infrastructure> Infrastructure;
	std::unique_ptr<System_Debug> Debug;
};

extern Core_System* g_pSystem;