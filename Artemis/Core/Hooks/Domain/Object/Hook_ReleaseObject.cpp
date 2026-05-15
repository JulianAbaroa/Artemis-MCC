#include "pch.h"

// Header.
#include "Hook_ReleaseObject.h"

// Types.
#include "Core/Types/Infrastructure/AOB/Signatures.h"

// --- Systems ---

#include "Core/Systems/Domain/Object/System_ObjectTable.h"

#include "Core/Systems/Infrastructure/Engine/Memory/System_AOBScanner.h"

#include "Core/Systems/Interface/Debug/System_Debug.h"

// MinHook.
#include "External/minhook/include/MinHook.h"

void Hook_ReleaseObject::HookedReleaseObject(unsigned int handle)
{
	m_OriginalFunction(handle);
	
	s_Instance->m_Deps.System_ObjectTable.OnObjectDestroyed(handle);
}

Hook_ReleaseObject* Hook_ReleaseObject::s_Instance = nullptr;

void Hook_ReleaseObject::Install()
{
	if (m_IsHookInstalled.load()) return;
	s_Instance = this;

	void* functionAddress = (void*)s_Instance->m_Deps.
		System_AOBScanner.FindPattern(Signatures::ReleaseObject);

	if (!functionAddress)
	{
		s_Instance->m_Deps.System_Debug.Log("[ReleaseObject] ERROR:"
			" Failed to obtain the function address.");
		return;
	}

	m_FunctionAddress.store(functionAddress);
	if (MH_CreateHook(m_FunctionAddress.load(), 
			&this->HookedReleaseObject, 
			reinterpret_cast<LPVOID*>(&m_OriginalFunction)
		) != MH_OK)
	{
		s_Instance->m_Deps.System_Debug.Log("[ReleaseObject] ERROR:"
			" Failed to create the hook.");
		return;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
	{
		s_Instance->m_Deps.System_Debug.Log("[ReleaseObject] ERROR:"
			" Failed to enable hook.");
		return;
	}

	m_IsHookInstalled.store(true);
	s_Instance->m_Deps.System_Debug.Log("[ReleaseObject] INFO:"
		" Hook installed.");
	return;
}

void Hook_ReleaseObject::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);

	s_Instance->m_Deps.System_Debug.Log("[ReleaseObject] INFO:"
		" Hook uninstalled.");

	s_Instance = nullptr;
}