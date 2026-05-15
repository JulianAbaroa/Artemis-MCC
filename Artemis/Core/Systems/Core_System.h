#pragma once

#include <memory>

struct Core_State;

struct Core_System_Domain;
struct Core_System_Infrastructure;
struct Core_System_Interface;

// Main container for the application's systems.
struct Core_System
{
	Core_System();
	~Core_System();

	void Initialize(Core_State& state);
	void Shutdown() const;

	std::unique_ptr<Core_System_Domain> Domain;
	std::unique_ptr<Core_System_Infrastructure> Infrastructure;
	std::unique_ptr<Core_System_Interface> Interface;
};