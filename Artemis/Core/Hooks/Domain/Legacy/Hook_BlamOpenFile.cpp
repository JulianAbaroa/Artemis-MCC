#include "pch.h"
#include "Core/Hooks/Domain/Legacy/Hook_BlamOpenFile.h"
#include "Core/States/Core_State.h"
#include "Core/States/Domain/Core_State_Domain.h"
#include "Core/States/Infrastructure/Core_State_Infrastructure.h"
#include "Core/States/Infrastructure/Engine/State_Lifecycle.h"
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Infrastructure/Core_System_Infrastructure.h"
#include "Core/Systems/Infrastructure/Engine/System_Scanner.h"
#include "Core/Systems/Interface/System_Debug.h"
#include "Core/Threads/Core_Thread.h"
#include "Core/Threads/Domain/Thread_Main.h"
#include "External/minhook/include/MinHook.h"

// This function is a hook for the Blam! File System API, used for interacting with Windows I/O.
// It is triggered during engine initialization and state transitions to load maps,
// game variants, and resource packages.
void Hook_BlamOpenFile::HookedBlamOpenFile(
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

bool Hook_BlamOpenFile::Install()
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

void Hook_BlamOpenFile::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);
	g_pSystem->Debug->Log("[BlamOpenFile] INFO: Hook uninstalled.");
}