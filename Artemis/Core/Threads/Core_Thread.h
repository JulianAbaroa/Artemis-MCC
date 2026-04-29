#pragma once

#include <memory>

class Thread_Main;
class Thread_Input;

class Thread_AI;

// Main container for the application's threads.
struct Core_Thread
{
	Core_Thread();
	~Core_Thread();

	std::unique_ptr<Thread_Main> Main;
	std::unique_ptr<Thread_Input> Input;

	std::unique_ptr<Thread_AI> AI;
};

extern Core_Thread* g_pThread;