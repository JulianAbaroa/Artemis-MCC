#pragma once

#include <memory>

struct Core_State_Domain;
struct Core_State_Infrastructure;
class State_Debug;

// Main container for the application's states.
struct Core_State
{
	Core_State();
	~Core_State();

	std::unique_ptr<Core_State_Domain> Domain;
	std::unique_ptr<Core_State_Infrastructure> Infrastructure;
	std::unique_ptr<State_Debug> Debug;
};

extern Core_State* g_pState;