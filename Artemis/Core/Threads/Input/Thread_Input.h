#pragma once

class State_Settings;
class State_Lifecycle;
class System_Input;
class System_Logs;

struct Thread_Input_Dependencies
{
	State_Settings& State_Settings;
	State_Lifecycle& State_Lifecycle;
	System_Input& System_Input;
	System_Logs& System_Logs;
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