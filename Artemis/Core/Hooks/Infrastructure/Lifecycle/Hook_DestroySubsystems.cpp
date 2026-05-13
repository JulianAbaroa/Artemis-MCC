#include "pch.h"

// Header.
#include "Hook_DestroySubsystems.h"

// --- Hooks ---
#include "Core/Hooks/Core_Hook.h"
#include "Core/Hooks/Domain/Core_Hook_Domain.h"
#include "Core/Hooks/Infrastructure/Core_Hook_Infrastructure.h"

#include "Core/Hooks/Domain/Map/Hook_BlamOpenMap.h"
#include "Core/Hooks/Domain/Object/Hook_CreateObject.h"
#include "Core/Hooks/Domain/Object/Hook_ReleaseObject.h"
#include "Core/Hooks/Domain/Player/Hook_CreatePlayer.h"

#include "../Input/Hook_GetButtonState.h"

// --- States ---
#include "Core/States/Core_State.h"
#include "Core/States/Infrastructure/Core_State_Infrastructure.h"

#include "Core/States/Infrastructure/Engine/Lifecycle/State_Lifecycle.h"

// Systems.
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Domain/Core_System_Domain.h"
#include "Core/Systems/Infrastructure/Core_System_Infrastructure.h"

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

#include "Core/Systems/Interface/System_Debug.h"

// --- UI ---
#include "Core/UI/Core_UI.h"

#include "Core/UI/Domain/Object/UI_ObjectTable.h"
#include "Core/UI/Domain/Player/UI_PlayerTable.h"
#include "Core/UI/Domain/Graph/UI_ObjectGraph.h"
#include "Core/UI/Domain/Navigation/UI_Navigation.h"
#include "Core/UI/Domain/Interactable/UI_Interactable.h"

// MinHook.
#include "External/minhook/include/MinHook.h"

void __fastcall Hook_DestroySubsystems::HookedDestroySubsystems(void)
{
	// --- Hooks ---

	g_pHook->Domain->BlamOpenMap->Uninstall();
	g_pHook->Domain->CreateGO->Uninstall();
	g_pHook->Domain->ReleaseGO->Uninstall();
	g_pHook->Domain->CreatePlayer->Uninstall();
	
	g_pHook->Infrastructure->GetButtonState->Uninstall();

	// --- Systems ---

	g_pSystem->Domain->Map->Cleanup();
	g_pSystem->Domain->MapTagGroup->Cleanup();
	g_pSystem->Domain->ObjectTable->Cleanup();
	g_pSystem->Domain->PlayerTable->Cleanup();
	g_pSystem->Domain->InteractionTable->Cleanup();
	g_pSystem->Domain->ObjectClassifier->Cleanup();
	g_pSystem->Domain->ObjectGraph->Cleanup();
	g_pSystem->Domain->PlayerGraph->Cleanup();
	g_pSystem->Domain->Navigation->Cleanup();
	g_pSystem->Domain->Environment->Cleanup();
	g_pSystem->Domain->Interactable->Cleanup();

	// --- UI ---

	g_pUI->ObjectTable->Cleanup();
	g_pUI->PlayerTable->Cleanup();
	g_pUI->ObjectGraph->Cleanup();
	g_pUI->Navigation->Cleanup();
	g_pUI->Interactable->Cleanup();
	
	g_pState->Infrastructure->Lifecycle->SetEngineStatus(
		{ EngineStatus::Destroyed });

	m_OriginalFunction();

	auto& debug = *g_pSystem->Debug;
	debug.Log("[DestroySubsystems] INFO: Game engine destroyed.");
}

bool Hook_DestroySubsystems::Install()
{
	if (m_IsHookInstalled.load()) return true;

	auto& debug = *g_pSystem->Debug;
	auto& aobScanner = *g_pSystem->Infrastructure->AOBScanner;

	void* functionAddress = 
		(void*)aobScanner.FindPattern(Signatures::DestroySubsystems);

	if (!functionAddress) return false;

	m_FunctionAddress.store(functionAddress);
	MH_RemoveHook(m_FunctionAddress.load());

	if (MH_CreateHook(
			m_FunctionAddress.load(), 
			&HookedDestroySubsystems, 
			reinterpret_cast<LPVOID*>(&m_OriginalFunction)) 
		!= MH_OK)
	{
		debug.Log("[DestroySubsystems] ERROR: Failed to create the hook.");
		return false;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK) 
	{
		debug.Log("[DestroySubsystems] ERROR: Failed to enable the hook.");
		return false;
	}

	m_IsHookInstalled.store(true);
	debug.Log("[DestroySubsystems] INFO: Hook installed.");
	return true;
}

void Hook_DestroySubsystems::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);

	auto& debug = *g_pSystem->Debug;
	debug.Log("[DestroySubsystems] INFO: Hook uninstalled.");
}

void* Hook_DestroySubsystems::GetFunctionAddress()
{
	return m_FunctionAddress.load();
}