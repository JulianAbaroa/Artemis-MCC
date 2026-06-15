#pragma once

#include <chrono>

class Hook_EngineInitialize;
class Hook_DestroySubsystems;
class Hook_ResizeBuffers;
class Hook_Present;
class State_Lifecycle;
class System_Telemetry;
class System_Logs;

struct Thread_Main_Dependencies
{
	Hook_EngineInitialize& Hook_EngineInitialize;
	Hook_DestroySubsystems& Hook_DestroySubsystems;
	Hook_ResizeBuffers& Hook_ResizeBuffers;
	Hook_Present& Hook_Present;
	State_Lifecycle& State_Lifecycle;
	System_Telemetry& System_Telemetry;
	System_Logs& System_Logs;
};

class Thread_Main
{
public:
	Thread_Main(Thread_Main_Dependencies dependencies) : 
		m_Deps(dependencies) {}
	~Thread_Main() = default;

	void Run();
	
private:
	Thread_Main_Dependencies m_Deps;

	bool WaitOrExit(std::chrono::milliseconds ms);

	bool InstallLifecycleHooks();
	void Shutdown();

	void CheckHooksHealth();
	bool IsHookIntact(void* address);

	bool IsStillRunning();
};