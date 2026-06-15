#pragma once

class State_Lifecycle;
class System_Logs;

struct Sys_Lifecycle_Deps
{
	State_Lifecycle& State_Lifecycle;
	System_Logs& System_Logs;
};

class System_Lifecycle
{
public:
	System_Lifecycle(Sys_Lifecycle_Deps deps) : m_Deps(deps) {}
	~System_Lifecycle() = default;

	void SignalShutdown();

private:
	Sys_Lifecycle_Deps m_Deps;
};