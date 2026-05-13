#include "pch.h"

// Header.
#include "Hook_CreateObject.h"

// --- Systems ---
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Domain/Core_System_Domain.h"
#include "Core/Systems/Infrastructure/Core_System_Infrastructure.h"

#include "Core/Systems/Domain/Object/System_ObjectTable.h"

#include "Core/Systems/Infrastructure/Engine/Memory/System_AOBScanner.h"

#include "Core/Systems/Interface/System_Debug.h"

// MinHook.
#include "External/minhook/include/MinHook.h"

unsigned long long __fastcall Hook_CreateObject::HookedCreateObject(unsigned short* placementData)
{
	auto handle = m_OriginalFunction(placementData);

	uint32_t datumIndex = 
		*(uint32_t*)((unsigned char*)placementData + 0x00);

	auto& objectTable = *g_pSystem->Domain->ObjectTable;
	objectTable.OnObjectCreated(static_cast<uint32_t>(handle), datumIndex);

	return handle;
}

void Hook_CreateObject::Install()
{
	if (m_IsHookInstalled.load()) return;

	auto& debug = *g_pSystem->Debug;
	auto& aobScanner = *g_pSystem->Infrastructure->AOBScanner;

	void* functionAddress = 
		(void*)aobScanner.FindPattern(Signatures::CreateObject);

	if (!functionAddress)
	{
		debug.Log("[CreateObject] ERROR: Failed to obtain"
			" the function address.");
		return;
	}

	m_FunctionAddress.store(functionAddress);
	if (MH_CreateHook(m_FunctionAddress.load(), 
			&this->HookedCreateObject, 
			reinterpret_cast<LPVOID*>(&m_OriginalFunction)
		) != MH_OK)
	{
		debug.Log("[CreateObject] ERROR: Failed to create the hook.");
		return;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
	{
		debug.Log(" [CreateObject] ERROR: Failed to enable hook.");
		return;
	}

	m_IsHookInstalled.store(true);
	debug.Log("[CreateObject] INFO: Hook installed.");
	return;
}

void Hook_CreateObject::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);

	auto& debug = *g_pSystem->Debug;
	debug.Log("[CreateObject] INFO: Hook uninstalled.");
}