#pragma once

class State_Lifecycle;
class System_Debug;

struct System_Lifecycle_Dependencies
{
	State_Lifecycle& State_Lifecycle;
	System_Debug& System_Debug;
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