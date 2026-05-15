#include "pch.h"

// Header.
#include "Hook_DestroySubsystems.h"

// Types.
#include "Core/Types/Infrastructure/AOB/Signatures.h"

// --- Hooks ---

#include "Core/Hooks/Domain/Map/Hook_BlamOpenMap.h"
#include "Core/Hooks/Domain/Object/Hook_CreateObject.h"
#include "Core/Hooks/Domain/Object/Hook_ReleaseObject.h"
#include "Core/Hooks/Domain/Player/Hook_CreatePlayer.h"

#include "Core/Hooks/Infrastructure/Input/Hook_GetButtonState.h"

// --- States ---

#include "Core/States/Infrastructure/Engine/Lifecycle/State_Lifecycle.h"

// --- Systems ---

#include "Core/Systems/Domain/Map/System_Map.h"
#include "Core/Systems/Domain/Map/System_MapTagGroup.h"
#include "Core/Systems/Domain/Object/System_ObjectTable.h"
#include "Core/Systems/Domain/Player/System_PlayerTable.h"
#include "Core/Systems/Domain/Interaction/System_InteractionTable.h"
#include "Core/Systems/Domain/Classification/System_ObjectClassifier.h"
#include "Core/Systems/Domain/Graph/System_ObjectGraph.h"
#include "Core/Systems/Domain/Graph/System_PlayerGraph.h"
#include "Core/Systems/Domain/Navigation/System_Navigation.h"
#include "Core/Systems/Domain/Environment/System_Environment.h"
#include "Core/Systems/Domain/Interactable/System_Interactable.h"

#include "Core/Systems/Infrastructure/Engine/Memory/System_AOBScanner.h"

#include "Core/Systems/Interface/Debug/System_Debug.h"

// --- UI ---

#include "Core/UI/Domain/Object/UI_ObjectTable.h"
#include "Core/UI/Domain/Player/UI_PlayerTable.h"
#include "Core/UI/Domain/Graph/UI_ObjectGraph.h"
#include "Core/UI/Domain/Interactable/UI_Interactable.h"
#include "Core/UI/Domain/Map/UI_Map.h"

// MinHook.
#include "External/minhook/include/MinHook.h"

void __fastcall Hook_DestroySubsystems::HookedDestroySubsystems(void)
{
	// --- Hooks ---

	s_Instance->m_Deps.Hook_BlamOpenMap.Uninstall();
	s_Instance->m_Deps.Hook_CreateObject.Uninstall();
	s_Instance->m_Deps.Hook_ReleaseObject.Uninstall();
	s_Instance->m_Deps.Hook_CreatePlayer.Uninstall();
	
	s_Instance->m_Deps.Hook_GetButtonState.Uninstall();

	// --- Systems ---

	s_Instance->m_Deps.System_Map.Cleanup();
	s_Instance->m_Deps.System_MapTagGroup.Cleanup();
	s_Instance->m_Deps.System_ObjectTable.Cleanup();
	s_Instance->m_Deps.System_PlayerTable.Cleanup();
	s_Instance->m_Deps.System_InteractionTable.Cleanup();
	s_Instance->m_Deps.System_ObjectClassifier.Cleanup();
	s_Instance->m_Deps.System_ObjectGraph.Cleanup();
	s_Instance->m_Deps.System_PlayerGraph.Cleanup();
	s_Instance->m_Deps.System_Navigation.Cleanup();
	s_Instance->m_Deps.System_Environment.Cleanup();
	s_Instance->m_Deps.System_Interactable.Cleanup();

	// --- UI ---
	s_Instance->m_Deps.UI_ObjectTable.Cleanup();
	s_Instance->m_Deps.UI_PlayerTable.Cleanup();
	s_Instance->m_Deps.UI_ObjectGraph.Cleanup();
	s_Instance->m_Deps.UI_Interactable.Cleanup();
	s_Instance->m_Deps.UI_Map.Cleanup();

	s_Instance->m_Deps.State_Lifecycle.SetEngineStatus(
		{ EngineStatus::Destroyed });

	m_OriginalFunction();

	s_Instance->m_Deps.System_Debug.Log("[DestroySubsystems] INFO:"
		" Game engine destroyed.");
}

Hook_DestroySubsystems* Hook_DestroySubsystems::s_Instance = nullptr;

bool Hook_DestroySubsystems::Install()
{
	if (m_IsHookInstalled.load()) return true;
	s_Instance = this;

	void* functionAddress = (void*)s_Instance->m_Deps.System_AOBScanner.
		FindPattern(Signatures::DestroySubsystems);

	if (!functionAddress) return false;

	m_FunctionAddress.store(functionAddress);
	MH_RemoveHook(m_FunctionAddress.load());

	if (MH_CreateHook(
			m_FunctionAddress.load(), 
			&HookedDestroySubsystems, 
			reinterpret_cast<LPVOID*>(&m_OriginalFunction)) 
		!= MH_OK)
	{
		s_Instance->m_Deps.System_Debug.Log("[DestroySubsystems] ERROR:"
			" Failed to create the hook.");
		return false;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK) 
	{
		s_Instance->m_Deps.System_Debug.Log("[DestroySubsystems] ERROR:"
			" Failed to enable the hook.");
		return false;
	}

	m_IsHookInstalled.store(true);
	s_Instance->m_Deps.System_Debug.Log("[DestroySubsystems] INFO:"
		" Hook installed.");
	return true;
}

void Hook_DestroySubsystems::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);

	s_Instance->m_Deps.System_Debug.Log("[DestroySubsystems] INFO:"
		" Hook uninstalled.");

	s_Instance = nullptr;
}

void* Hook_DestroySubsystems::GetFunctionAddress()
{
	return m_FunctionAddress.load();
}