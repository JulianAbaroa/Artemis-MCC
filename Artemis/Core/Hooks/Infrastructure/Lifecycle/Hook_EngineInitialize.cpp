#include "pch.h"

// Head.
#include "Hook_EngineInitialize.h"

// Types.
#include "Core/Types/Infrastructure/AOB/Signatures.h"

// --- Hooks ---

#include "Core/Hooks/Domain/Map/Hook_BlamOpenMap.h"
#include "Core/Hooks/Domain/Object/Hook_ObjectTable.h"
#include "Core/Hooks/Domain/Object/Hook_CreateObject.h"
#include "Core/Hooks/Domain/Object/Hook_ReleaseObject.h"
#include "Core/Hooks/Domain/Player/Hook_PlayerTable.h"
#include "Core/Hooks/Domain/Player/Hook_CreatePlayer.h"
#include "Core/Hooks/Domain/Interaction/Hook_InteractionTable.h"

#include "Core/Hooks/Infrastructure/Input/Hook_GetButtonState.h"

// --- States ---

#include "Core/States/Infrastructure/Engine/Lifecycle/State_Lifecycle.h"

// --- Systems ---

#include "Core/Systems/Infrastructure/Engine/Memory/System_AOBScanner.h"

#include "Core/Systems/Interface/Debug/System_Debug.h"

// MinHook.
#include "External/minhook/include/MinHook.h"

void __fastcall Hook_EngineInitialize::HookedEngineInitialize(void)
{
	m_OriginalFunction();

	s_Instance->m_Deps.Hook_BlamOpenMap.Install();
	s_Instance->m_Deps.Hook_CreateObject.Install();
	s_Instance->m_Deps.Hook_ReleaseObject.Install();
	s_Instance->m_Deps.Hook_CreatePlayer.Install();

	s_Instance->m_Deps.Hook_GetButtonState.Install();

	s_Instance->m_Deps.Hook_ObjectTable.FindAndStoreTableBase();
	s_Instance->m_Deps.Hook_PlayerTable.FindAndStoreTableBase();
	s_Instance->m_Deps.Hook_InteractionTable.FindAndStoreTableBase();
	
	s_Instance->m_Deps.State_Lifecycle.SetEngineStatus(
		{ EngineStatus::Running });

	s_Instance->m_Deps.System_Debug.Log("[EngineInitialize] INFO:"
		" Game engine initialized.");
}

Hook_EngineInitialize* Hook_EngineInitialize::s_Instance = nullptr;

bool Hook_EngineInitialize::Install()
{
	if (m_IsHookInstalled.load()) return true;
	s_Instance = this;

	void* functionAddress = (void*)s_Instance->m_Deps.System_AOBScanner.
		FindPattern(Signatures::EngineInitialize);
	
	if (!functionAddress) return false;

	m_FunctionAddress.store(functionAddress);
	MH_RemoveHook(m_FunctionAddress.load());

	if (MH_CreateHook(
			m_FunctionAddress.load(), 
		&this->HookedEngineInitialize, 
		reinterpret_cast<LPVOID*>(&m_OriginalFunction)) 
		!= MH_OK)
	{
		s_Instance->m_Deps.System_Debug.Log("[EngineInitialize] ERROR:"
			" Failed to create the hook.");
		return false;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK) 
	{
		s_Instance->m_Deps.System_Debug.Log("[EngineInitialize] ERROR:"
			" Failed to enable the hook.");
		return false;
	}

	m_IsHookInstalled.store(true);
	s_Instance->m_Deps.System_Debug.Log("[EngineInitialize] INFO:"
		" Hook installed.");
	return true;
}

void Hook_EngineInitialize::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);

	s_Instance->m_Deps.System_Debug.Log("[EngineInitialize] INFO:"
		" Hook uninstalled.");

	s_Instance = nullptr;
}

void* Hook_EngineInitialize::GetFunctionAddress()
{
	return m_FunctionAddress.load();
}