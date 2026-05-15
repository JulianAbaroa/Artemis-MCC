#pragma once

#include <chrono>

class State_Lifecycle;

class System_Thread
{
public:
	System_Thread(State_Lifecycle& stateLifecycle) : 
		State_Lifecycle(stateLifecycle) {}
	~System_Thread() = default;

	bool WaitOrExit(std::chrono::milliseconds ms);

private:
	State_Lifecycle& State_Lifecycle;
};