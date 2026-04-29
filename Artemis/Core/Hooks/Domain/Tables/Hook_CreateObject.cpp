#include "pch.h"
#include "Core/Hooks/Domain/Tables/Hook_CreateObject.h"
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Domain/Core_System_Domain.h"
#include "Core/Systems/Domain/Tables/System_ObjectTable.h"
#include "Core/Systems/Infrastructure/Core_System_Infrastructure.h"
#include "Core/Systems/Infrastructure/Engine/System_Scanner.h"
#include "Core/Systems/Interface/System_Debug.h"
#include "External/minhook/include/MinHook.h"

unsigned long long __fastcall Hook_CreateObject::HookedCreateObject(unsigned short* placementData)
{
	auto handle = m_OriginalFunction(placementData);
	uint32_t datumIndex = *(uint32_t*)((unsigned char*)placementData + 0x00);

	g_pSystem->Domain->ObjectTable->OnObjectCreated(
		static_cast<uint32_t>(handle), datumIndex);

	return handle;
}

void Hook_CreateObject::Install()
{
	if (m_IsHookInstalled.load()) return;

	void* functionAddress = 
		(void*)g_pSystem->Infrastructure->Scanner->FindPattern(
			Signatures::CreateObject);

	if (!functionAddress)
	{
		g_pSystem->Debug->Log("[CreateObject] ERROR:"
			" Failed to obtain the function address.");

		return;
	}

	m_FunctionAddress.store(functionAddress);
	if (MH_CreateHook(m_FunctionAddress.load(), 
			&this->HookedCreateObject, 
			reinterpret_cast<LPVOID*>(&m_OriginalFunction)
		) != MH_OK)
	{
		g_pSystem->Debug->Log("[CreateObject] ERROR:"
			" Failed to create the hook.");

		return;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
	{
		g_pSystem->Debug->Log(" [CreateObject] ERROR:"
			" Failed to enable hook.");

		return;
	}

	m_IsHookInstalled.store(true);
	g_pSystem->Debug->Log("[CreateObject] INFO: Hook installed.");
	return;
}

void Hook_CreateObject::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);
	g_pSystem->Debug->Log("[CreateObject] INFO: Hook uninstalled.");
}