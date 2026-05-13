#pragma once

// Types.
#include "Core/Types/Infrastructure/EngineStatus.h"

class Thread_Main
{
public:
	void Run();
	
private:
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