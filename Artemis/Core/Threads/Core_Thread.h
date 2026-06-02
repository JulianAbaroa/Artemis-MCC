#pragma once

#include <memory>

class Core_State;
class Core_System;
class Core_Hook;

class Thread_Main;
class Thread_AI;
class Thread_Input;

class Core_Thread
{
public:
	Core_Thread();
	~Core_Thread();

	void Initialize(Core_State& state, Core_System& system, Core_Hook& hook);
	void Deinitialize();

	std::unique_ptr<Thread_Main> Main;
	std::unique_ptr<Thread_AI> AI;
	std::unique_ptr<Thread_Input> Input;
};