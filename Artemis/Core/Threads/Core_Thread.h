#pragma once

#include <memory>

struct Core_State;
struct Core_System;
struct Core_Hook;

class Thread_Main;
class Thread_AI;
class Thread_Input;

// Main container for the application's threads.
struct Core_Thread
{
	Core_Thread();
	~Core_Thread();

	void Initialize(Core_State& state, Core_System& system, Core_Hook& hook);
	void Shutdown();

	std::unique_ptr<Thread_Main> Main;
	std::unique_ptr<Thread_AI> AI;
	std::unique_ptr<Thread_Input> Input;
};