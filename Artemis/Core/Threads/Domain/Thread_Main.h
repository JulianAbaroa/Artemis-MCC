#pragma once

class Hook_EngineInitialize;
class Hook_DestroySubsystems;
class Hook_GameEngineInit;
class Hook_Present;
class Hook_ResizeBuffers;
class State_Lifecycle;
class System_Thread;
class System_Debug;

struct Thread_Main_Dependencies
{
	Hook_EngineInitialize& Hook_EngineInitialize;
	Hook_DestroySubsystems& Hook_DestroySubsystems;
	Hook_GameEngineInit& Hook_GameEngineInit;
	Hook_Present& Hook_Present;
	Hook_ResizeBuffers& Hook_ResizeBuffers;
	State_Lifecycle& State_Lifecycle;
	System_Thread& System_Thread;
	System_Debug& System_Debug;
};

class Thread_Main
{
public:
	Thread_Main(Thread_Main_Dependencies dependencies) : m_Deps(dependencies){}
	~Thread_Main() = default;

	void Run();
	
private:
	Thread_Main_Dependencies m_Deps;

	void InitializeArtemis();
	void DeinitializeArtemis();

	void InstallUIHooks();
	void UninstallUIHooks();

	void CheckHooksHealth();
	bool IsStillRunning();

	bool TryInstallLifecycleHooks(const char* context);
	bool IsHookIntact(void* address);
	void Shutdown();
};