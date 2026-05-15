#include "pch.h"

// Header.
#include "Hook_GameEngineInit.h"

// Types.
#include "Core/Types/Infrastructure/AOB/Signatures.h"

// --- Systems ---

#include "Core/Systems/Infrastructure/Engine/Memory/System_AOBScanner.h"

#include "Core/Systems/Interface/Debug/System_Debug.h"

// MinHook.
#include "External/minhook/include/MinHook.h"

void __fastcall Hook_GameEngineInit::HookedGameEngineInit(
	uint64_t param_1, uint64_t pSystem, uint64_t* pConfiguration)
{
	m_OriginalFunction(param_1, pSystem, pConfiguration);
}

Hook_GameEngineInit* Hook_GameEngineInit::s_Instance = nullptr;

bool Hook_GameEngineInit::Install()
{
	if (m_IsHookInstalled.load()) return true;
	s_Instance = this;

	void* functionAddress = (void*)s_Instance->m_Deps.
		System_AOBScanner.FindPattern(Signatures::GameEngineStart);

	if (!functionAddress) return false;

	m_FunctionAddress.store(functionAddress);
	MH_RemoveHook(m_FunctionAddress.load());

	if (MH_CreateHook(
			m_FunctionAddress.load(), 
			&this->HookedGameEngineInit, 
			reinterpret_cast<LPVOID*>(&m_OriginalFunction)) 
		!= MH_OK)
	{
		s_Instance->m_Deps.System_Debug.Log("[GameEngineStart] ERROR:"
			" Failed to create the hook.");
		return false;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK) 
	{
		s_Instance->m_Deps.System_Debug.Log("[GameEngineStart] ERROR:"
			" Failed to enable the hook.");
		return false;
	}

	m_IsHookInstalled.store(true);
	s_Instance->m_Deps.System_Debug.Log("[GameEngineStart] INFO:"
		" Hook installed.");

	return true;
}

void Hook_GameEngineInit::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);

	s_Instance->m_Deps.System_Debug.Log("[GameEngineStart] INFO:"
		" Hook uninstalled.");

	s_Instance = nullptr;
}

void* Hook_GameEngineInit::GetFunctionAddress()
{
	return m_FunctionAddress.load();
}