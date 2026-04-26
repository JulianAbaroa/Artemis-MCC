#include "pch.h"
#include "Core/Hooks/Data/Legacy/BlamOpenFileHook.h"
#include "Core/States/CoreState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Engine/LifecycleState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Engine/ScannerSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include "Core/Threads/CoreThread.h"
#include "Core/Threads/Domain/MainThread.h"
#include "External/minhook/include/MinHook.h"

// This function is a hook for the Blam! File System API, used for interacting with Windows I/O.
// It is triggered during engine initialization and state transitions to load maps,
// game variants, and resource packages.
void BlamOpenFileHook::HookedBlamOpenFile(
	long long fileContext, uint32_t accessFlags, uint32_t* translatedStatus)
{
	m_OriginalFunction(fileContext, accessFlags, translatedStatus);

	const char* filePath = (const char*)((uintptr_t)fileContext + 0x8);

	if (filePath != nullptr && !IsBadReadPtr(filePath, 4)) 
	{
		std::string pathStr(filePath);

		g_pSystem->Debug->Log("[BlamOpenFile] INFO: Path: %s", pathStr.c_str());
	}
}

bool BlamOpenFileHook::Install()
{
	if (m_IsHookInstalled.load()) return false;

	void* functionAddress = {}; //(void*)g_pSystem->Infrastructure->Scanner->FindPattern(Signatures::BlamOpenFile);
	if (!functionAddress)
	{
		g_pSystem->Debug->Log("[BlamOpenFile] ERROR: Failed to obtain the function address.");
		return false;
	}

	m_FunctionAddress.store(functionAddress);
	if (MH_CreateHook(m_FunctionAddress.load(), &this->HookedBlamOpenFile, reinterpret_cast<LPVOID*>(&m_OriginalFunction)) != MH_OK)
	{
		g_pSystem->Debug->Log("[BlamOpenFile] ERROR: Failed to create the hook.");
		return false;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
	{
		g_pSystem->Debug->Log(" [BlamOpenFile] ERROR: Failed to enable hook.");
		return false;
	}

	m_IsHookInstalled.store(true);
	g_pSystem->Debug->Log("[BlamOpenFile] INFO: Hook installed.");
	return true;
}

void BlamOpenFileHook::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);
	g_pSystem->Debug->Log("[BlamOpenFile] INFO: Hook uninstalled.");
}