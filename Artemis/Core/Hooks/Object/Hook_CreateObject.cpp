#include "pch.h"

#include "Hook_CreateObject.h"

#include "Core/Types/AOB/Signatures.h"

#include "Core/Systems/Tables/Object/System_ObjectTable.h"
#include "Core/Systems/Memory/AOBScanner/System_AOBScanner.h"
#include "Core/Systems/Logs/System_Logs.h"

#include "External/minhook/include/MinHook.h"

unsigned long long __fastcall Hook_CreateObject::HookedCreateObject(unsigned short* placementData)
{
	auto handle = m_OriginalFunction(placementData);

	uint32_t datumIndex = 
		*(uint32_t*)((unsigned char*)placementData + 0x00);

	s_Instance->m_Deps.System_ObjectTable.OnObjectCreated(
		static_cast<uint32_t>(handle), datumIndex);

	return handle;
}

Hook_CreateObject* Hook_CreateObject::s_Instance = nullptr;

void Hook_CreateObject::Install()
{
	if (m_IsHookInstalled.load()) return;
	s_Instance = this;

	void* functionAddress = (void*)s_Instance->m_Deps.
		System_AOBScanner.FindPattern(Signatures::CreateObject);

	if (!functionAddress)
	{
		s_Instance->m_Deps.System_Logs.Log("[CreateObject] ERROR:"
			" Failed to obtain the function address.");
		return;
	}

	m_FunctionAddress.store(functionAddress);
	if (MH_CreateHook(m_FunctionAddress.load(), 
			&this->HookedCreateObject, 
			reinterpret_cast<LPVOID*>(&m_OriginalFunction)
		) != MH_OK)
	{
		s_Instance->m_Deps.System_Logs.Log("[CreateObject] ERROR:"
			" Failed to create the hook.");
		return;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
	{
		s_Instance->m_Deps.System_Logs.Log("[CreateObject] ERROR:"
			" Failed to enable hook.");
		return;
	}

	m_IsHookInstalled.store(true);
	s_Instance->m_Deps.System_Logs.Log("[CreateObject] INFO:"
		" Hook installed.");
	return;
}

void Hook_CreateObject::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);

	s_Instance->m_Deps.System_Logs.Log("[CreateObject] INFO:"
		" Hook uninstalled.");

	s_Instance = nullptr;
}