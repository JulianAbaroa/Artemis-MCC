#pragma once

#include <memory>

struct Core_Hook_Domain;
struct Core_Hook_Infrastructure;

// Main container for th application's hooks.
struct Core_Hook
{
	Core_Hook();
	~Core_Hook();

	std::unique_ptr<Core_Hook_Domain> Domain;
	std::unique_ptr<Core_Hook_Infrastructure> Infrastructure;
};

extern Core_Hook* g_pHook;