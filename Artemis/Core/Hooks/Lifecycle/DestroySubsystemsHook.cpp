#include "pch.h"
#include "Core/UI/CoreUI.h"
#include "Core/UI/Tabs/Domain/Tables/ObjectTableTab.h"
#include "Core/UI/Tabs/Domain/Tables/PlayerTableTab.h"
#include "Core/UI/Tabs/Domain/Graph/ObjectGraphTab.h"
#include "Core/UI/Tabs/Domain/Interactable/InteractableTab.h"
#include "Core/UI/Tabs/Domain/Physics/PhysicsDebugTab.h"

#include "Core/Hooks/CoreHook.h"
#include "Core/Hooks/Data/CoreDataHook.h"
#include "Core/Hooks/Data/Tables/CreateObjectHook.h"
#include "Core/Hooks/Data/Tables/ReleaseObjectHook.h"
#include "Core/Hooks/Data/Tables/CreatePlayerHook.h"
#include "Core/Hooks/Data/Map/BlamOpenMapHook.h"
//#include "Core/Hooks/Data/Legacy/BlamOpenFileHook.h"
#include "Core/Hooks/Data/Legacy/BuildGameEventHook.h"
#include "Core/Hooks/Input/CoreInputHook.h"
#include "Core/Hooks/Input/GetButtonStateHook.h"
#include "Core/Hooks/Lifecycle/DestroySubsystemsHook.h"

#include "Core/States/CoreState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Engine/LifecycleState.h"

#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Domain/CoreDomainSystem.h"
#include "Core/Systems/Domain/Map/MapSystem.h"
#include "Core/Systems/Domain/Map/MapTagGroupSystem.h"
#include "Core/Systems/Domain/Tables/ObjectTableSystem.h"
#include "Core/Systems/Domain/Tables/PlayerTableSystem.h"
#include "Core/Systems/Domain/Tables/InteractionTableSystem.h"
#include "Core/Systems/Domain/Graph/ObjectGraphSystem.h"
#include "Core/Systems/Domain/Interactable/InteractableSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Engine/ScannerSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include "External/minhook/include/MinHook.h"

void __fastcall DestroySubsystemsHook::HookedDestroySubsystems(void)
{
	// Hooks: Map.
	g_pHook->Data->BlamOpenMap->Uninstall();

	// Hooks: Tables.
	g_pHook->Data->CreateGO->Uninstall();
	g_pHook->Data->ReleaseGO->Uninstall();
	g_pHook->Data->CreatePlayer->Uninstall();

	// Hooks: Other.
	g_pHook->Data->BuildGameEvent->Uninstall();	
	g_pHook->Input->GetButtonState->Uninstall();

	// Systems: Map.
	g_pSystem->Domain->Map->Cleanup();
	g_pSystem->Domain->MapTagGroup->Cleanup();

	// Systems: Tables.
	g_pSystem->Domain->ObjectTable->Cleanup();
	g_pSystem->Domain->PlayerTable->Cleanup();
	g_pSystem->Domain->InteractionTable->Cleanup();

	// Systems: Graph.
	g_pSystem->Domain->ObjectGraph->Cleanup();

	// Systems: Interactables.
	g_pSystem->Domain->Interactable->Cleanup();

	// Systems: Tags.
	//g_pSystem->Domain->Phmo->Cleanup();
	//g_pSystem->Domain->Vehi->Cleanup();

	// UI.
	g_pUI->ObjectTable->Cleanup();
	g_pUI->PlayerTable->Cleanup();
	g_pUI->ObjectGraph->Cleanup();
	g_pUI->Interactable->Cleanup();
	g_pUI->PhysicsDebug->Cleanup();
	
	g_pState->Infrastructure->Lifecycle->SetEngineStatus(
		{ EngineStatus::Destroyed });

	m_OriginalFunction();

	g_pSystem->Debug->Log("[DestroySubsystems] INFO: Game engine destroyed.");
}

bool DestroySubsystemsHook::Install(bool silent)
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

void DestroySubsystemsHook::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);
	g_pSystem->Debug->Log("[DestroySubsystems] INFO: Hook uninstalled.");
}

void* DestroySubsystemsHook::GetFunctionAddress()
{
	return m_FunctionAddress.load();
}