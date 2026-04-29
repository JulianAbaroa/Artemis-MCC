#pragma once

#include "Core/Types/Domain/ModTypes.h"

class Thread_Main
{
public:
	void Run();
	
private:
	void InitializeArtemis();
	void DeinitializeArtemis();

	void InstallCaptureHooks();
	void UninstallCaptureHooks();

	void CheckHooksHealth();
	bool IsStillRunning();

	bool TryInstallLifecycleHooks(const char* context);
	bool IsHookIntact(void* address);
	void Shutdown();
};