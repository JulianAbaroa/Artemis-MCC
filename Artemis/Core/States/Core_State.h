#pragma once

#include <memory>

struct Core_State_Domain;
struct Core_State_Infrastructure;
struct Core_State_Interface;

// Main container for the application's states.
struct Core_State
{
	Core_State();
	~Core_State();

	void Initialize();
	void Shutdown() const;

	std::unique_ptr<Core_State_Domain> Domain;
	std::unique_ptr<Core_State_Infrastructure> Infrastructure;
	std::unique_ptr<Core_State_Interface> Interface;
};