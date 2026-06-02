#pragma once

class State_Lifecycle;
class System_Logs;

struct System_Lifecycle_Dependencies
{
	State_Lifecycle& State_Lifecycle;
	System_Logs& System_Logs;
};

class System_Lifecycle
{
public:
	System_Lifecycle(System_Lifecycle_Dependencies dependencies) : 
		m_Deps(dependencies) {}
	~System_Lifecycle() = default;

	void SignalShutdown();

private:
	System_Lifecycle_Dependencies m_Deps;
};