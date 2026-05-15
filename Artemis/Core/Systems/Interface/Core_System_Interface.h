#pragma once

#include <memory>

struct Core_State;

class System_Debug;

struct Core_System_Interface
{
	Core_System_Interface();
	~Core_System_Interface();

	void Initialize(Core_State& state);
	void Shutdown();

	// Debug.
	std::unique_ptr<System_Debug> Debug;
};