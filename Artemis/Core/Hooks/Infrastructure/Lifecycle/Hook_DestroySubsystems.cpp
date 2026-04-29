#include "pch.h"
#include "Core/UI/Core_UI.h"
#include "Core/UI/Domain/Tables/UI_ObjectTable.h"
#include "Core/UI/Domain/Tables/UI_PlayerTable.h"
#include "Core/UI/Domain/Graph/UI_ObjectGraph.h"
#include "Core/UI/Domain/Interactable/UI_Interactable.h"

#include "Core/Hooks/Core_Hook.h"
#include "Core/Hooks/Domain/Core_Hook_Domain.h"
#include "Core/Hooks/Domain/Tables/Hook_CreateObject.h"
#include "Core/Hooks/Domain/Tables/Hook_ReleaseObject.h"
#include "Core/Hooks/Domain/Tables/Hook_CreatePlayer.h"
#include "Core/Hooks/Domain/Map/Hook_BlamOpenMap.h"
//#include "Core/Hooks/Domain/Legacy/Hook_BlamOpenFile.h"
#include "Core/Hooks/Domain/Legacy/Hook_BuildGameEvent.h"
#include "Core/Hooks/Infrastructure/Core_Hook_Infrastructure.h"
#include "Core/Hooks/Infrastructure/Input/Hook_GetButtonState.h"
#include "Core/Hooks/Infrastructure/Lifecycle/Hook_DestroySubsystems.h"

#include "Core/States/Core_State.h"
#include "Core/States/Infrastructure/Core_State_Infrastructure.h"
#include "Core/States/Infrastructure/Engine/State_Lifecycle.h"

#include "Core/Systems/Core_System.h"
#include "Core/Systems/Domain/Core_System_Domain.h"
#include "Core/Systems/Domain/Map/System_Map.h"
#include "Core/Systems/Domain/Map/System_MapTagGroup.h"
#include "Core/Systems/Domain/Environment/System_Environment.h"
#include "Core/Systems/Domain/Navigation/System_Navigation.h"
#include "Core/Systems/Domain/Tables/System_ObjectTable.h"
#include "Core/Systems/Domain/Tables/System_PlayerTable.h"
#include "Core/Systems/Domain/Tables/System_InteractionTable.h"
#include "Core/Systems/Domain/Graph/System_ObjectGraph.h"
#include "Core/Systems/Domain/Interactable/System_Interactable.h"
#include "Core/Systems/Infrastructure/Core_System_Infrastructure.h"
#include "Core/Systems/Infrastructure/Engine/System_Scanner.h"
#include "Core/Systems/Interface/System_Debug.h"
#include "External/minhook/include/MinHook.h"

void __fastcall Hook_DestroySubsystems::HookedDestroySubsystems(void)
{
	// Hooks: Map.
	g_pHook->Domain->BlamOpenMap->Uninstall();

	// Hooks: Tables.
	g_pHook->Domain->CreateGO->Uninstall();
	g_pHook->Domain->ReleaseGO->Uninstall();
	g_pHook->Domain->CreatePlayer->Uninstall();

	// Hooks: Other.
	g_pHook->Domain->BuildGameEvent->Uninstall();
	g_pHook->Infrastructure->GetButtonState->Uninstall();

	// Systems: Map.
	g_pSystem->Domain->Map->Cleanup();
	g_pSystem->Domain->MapTagGroup->Cleanup();

	// Systems: Environment.
	g_pSystem->Domain->Environment->Cleanup();

	// Systems: Navigation.
	g_pSystem->Domain->Navigation->Cleanup();

	// Systems: Tables.
	g_pSystem->Domain->ObjectTable->Cleanup();
	g_pSystem->Domain->PlayerTable->Cleanup();
	g_pSystem->Domain->InteractionTable->Cleanup();

	// Systems: Graph.
	g_pSystem->Domain->ObjectGraph->Cleanup();

	// Systems: Interactables.
	g_pSystem->Domain->Interactable->Cleanup();

	// UI.
	g_pUI->ObjectTable->Cleanup();
	g_pUI->PlayerTable->Cleanup();
	g_pUI->ObjectGraph->Cleanup();
	g_pUI->Interactable->Cleanup();
	
	g_pState->Infrastructure->Lifecycle->SetEngineStatus(
		{ EngineStatus::Destroyed });

	m_OriginalFunction();

	g_pSystem->Debug->Log("[DestroySubsystems] INFO: Game engine destroyed.");
}

bool Hook_DestroySubsystems::Install(bool silent)
{
	if (m_IsHookInstalled.load()) return true;

	void* functionAddress = (void*)g_pSystem->Infrastructure->Scanner->FindPattern(Signatures::DestroySubsystems);
	if (!functionAddress)
	{
		if (!silent)  g_pSystem->Debug->Log("[DestroySubsystems] ERROR: Failed to obtain the function address.");
		return false;
	}

	m_FunctionAddress.store(functionAddress);
	MH_RemoveHook(m_FunctionAddress.load());
	if (MH_CreateHook(m_FunctionAddress.load(), &HookedDestroySubsystems, reinterpret_cast<LPVOID*>(&m_OriginalFunction)) != MH_OK)
	{
		g_pSystem->Debug->Log("[DestroySubsystems] ERROR: Failed to create the hook.");
		return false;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK) 
	{
		g_pSystem->Debug->Log("[DestroySubsystems] ERROR: Failed to enable the hook.");
		return false;
	}

	m_IsHookInstalled.store(true);
	g_pSystem->Debug->Log("[DestroySubsystems] INFO: Hook installed.");
	return true;
}

void Hook_DestroySubsystems::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);
	g_pSystem->Debug->Log("[DestroySubsystems] INFO: Hook uninstalled.");
}

void* Hook_DestroySubsystems::GetFunctionAddress()
{
	return m_FunctionAddress.load();
}