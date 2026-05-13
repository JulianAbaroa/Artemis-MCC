#include "pch.h"

// Header.
#include "Hook_ReleaseObject.h"

// Systems.
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Domain/Core_System_Domain.h"
#include "Core/Systems/Infrastructure/Core_System_Infrastructure.h"

#include "Core/Systems/Domain/Object/System_ObjectTable.h"

#include "Core/Systems/Infrastructure/Engine/Memory/System_AOBScanner.h"

#include "Core/Systems/Interface/System_Debug.h"

// MinHook.
#include "External/minhook/include/MinHook.h"

void Hook_ReleaseObject::HookedReleaseObject(unsigned int handle)
{
	m_OriginalFunction(handle);
	
	auto& objectTable = *g_pSystem->Domain->ObjectTable;
	objectTable.OnObjectDestroyed(handle);
}

void Hook_ReleaseObject::Install()
{
	if (m_IsHookInstalled.load()) return;

	auto& debug = *g_pSystem->Debug;
	auto& aobScanner = *g_pSystem->Infrastructure->AOBScanner;
	
	void* functionAddress = 
		(void*)aobScanner.FindPattern(Signatures::ReleaseObject);

	if (!functionAddress)
	{
		debug.Log("[ReleaseObject] ERROR: Failed to obtain"
			" the function address.");
		return;
	}

	m_FunctionAddress.store(functionAddress);
	if (MH_CreateHook(m_FunctionAddress.load(), 
			&this->HookedReleaseObject, 
			reinterpret_cast<LPVOID*>(&m_OriginalFunction)
		) != MH_OK)
	{
		debug.Log("[ReleaseObject] ERROR: Failed to create the hook.");
		return;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
	{
		debug.Log("[ReleaseObject] ERROR: Failed to enable hook.");
		return;
	}

	m_IsHookInstalled.store(true);
	debug.Log("[ReleaseObject] INFO: Hook installed.");
	return;
}

void Hook_ReleaseObject::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);

	auto& debug = *g_pSystem->Debug;
	debug.Log("[ReleaseObject] INFO: Hook uninstalled.");
}