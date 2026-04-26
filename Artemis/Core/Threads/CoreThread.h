#pragma once

#include <memory>

class MainThread;
class InputThread;

class AIThread;

// Main container for the application's threads.
struct CoreThread
{
	CoreThread();
	~CoreThread();

	std::unique_ptr<MainThread> Main;
	std::unique_ptr<InputThread> Input;

	std::unique_ptr<AIThread> AI;
};

extern CoreThread* g_pThread;