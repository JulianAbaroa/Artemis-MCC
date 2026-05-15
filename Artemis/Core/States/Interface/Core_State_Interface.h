#pragma once

#include <memory>

class State_Debug;

struct Core_State_Interface
{
	Core_State_Interface();
	~Core_State_Interface();

	void Initialize();
	void Shutdown();

	// Debug.
	std::unique_ptr<State_Debug> Debug;
};