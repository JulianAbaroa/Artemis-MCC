#pragma once

class State_Lifecycle;
class State_Settings;
class System_Input;
class System_Thread;
class System_Debug;

struct Thread_Input_Dependencies
{
	State_Lifecycle& State_Lifecycle;
	State_Settings& State_Settings;
	System_Input& System_Input;
	System_Thread& System_Thread;
	System_Debug& System_Debug;
};

class Thread_Input
{
public:
	Thread_Input(Thread_Input_Dependencies dependencies) : 
		m_Deps(dependencies) {}
	~Thread_Input() = default;

	void Run();

private:
	Thread_Input_Dependencies m_Deps;
};