#pragma once

#include <memory>

struct Core_State;
struct Core_System;
struct Core_UI;

struct Core_Hook_Domain;
struct Core_Hook_Infrastructure;

// Main container for the application's hooks.
struct Core_Hook
{
	Core_Hook();
	~Core_Hook();

	void Initialize(Core_State& state, Core_System& system, Core_UI& ui);
	void Shutdown() const;

	std::unique_ptr<Core_Hook_Domain> Domain;
	std::unique_ptr<Core_Hook_Infrastructure> Infrastructure;
};