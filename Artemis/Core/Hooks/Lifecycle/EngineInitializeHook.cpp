#include "pch.h"
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
#include "Core/Hooks/Lifecycle/EngineInitializeHook.h"

#include "Core/States/CoreState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Engine/LifecycleState.h"

#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Domain/CoreDomainSystem.h"
#include "Core/Systems/Domain/Tables/ObjectTableSystem.h"
#include "Core/Systems/Domain/Tables/PlayerTableSystem.h"
#include "Core/Systems/Domain/Tables/InteractionTableSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Engine/ScannerSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include "External/minhook/include/MinHook.h"

void __fastcall EngineInitializeHook::HookedEngineInitialize(void)
{
	m_OriginalFunction();

	// Hooks: Map.
	g_pHook->Data->BlamOpenMap->Install();

	// Hooks: Tables
	g_pHook->Data->CreateGO->Install();
	g_pHook->Data->ReleaseGO->Install();
	g_pHook->Data->CreatePlayer->Install();

	// Other.
	g_pHook->Data->BuildGameEvent->Install();
	g_pHook->Input->GetButtonState->Install();

	// Systems: Tables.
	g_pSystem->Domain->ObjectTable->FindObjectTableBase();
	g_pSystem->Domain->PlayerTable->FindPlayerTableBase();
	g_pSystem->Domain->InteractionTable->FindInteractionTableBase();
	
	g_pState->Infrastructure->Lifecycle->SetEngineStatus(
		{ EngineStatus::Running });

	g_pSystem->Debug->Log("[EngineInitialize] INFO: Game engine initialized.");
}

bool EngineInitializeHook::Install(bool silent)
{
	if (m_IsHookInstalled.load()) return true;

	void* functionAddress = (void*)g_pSystem->Infrastructure->Scanner->FindPattern(Signatures::EngineInitialize);
	if (!functionAddress)
	{
		if (!silent)  g_pSystem->Debug->Log("[EngineInitialize] ERROR: Failed to obtain the function address.");
		return false;
	}

	m_FunctionAddress.store(functionAddress);
	MH_RemoveHook(m_FunctionAddress.load());
	if (MH_CreateHook(m_FunctionAddress.load(), &this->HookedEngineInitialize, reinterpret_cast<LPVOID*>(&m_OriginalFunction)) != MH_OK)
	{
		g_pSystem->Debug->Log("[EngineInitialize] ERROR: Failed to create the hook.");
		return false;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK) 
	{
		g_pSystem->Debug->Log("[EngineInitialize] ERROR: Failed to enable the hook.");
		return false;
	}

	m_IsHookInstalled.store(true);
	g_pSystem->Debug->Log("[EngineInitialize] INFO: Hook installed.");
	return true;
}

void EngineInitializeHook::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);
	g_pSystem->Debug->Log("[EngineInitialize] INFO: Hook uninstalled.");
}

void* EngineInitializeHook::GetFunctionAddress()
{
	return m_FunctionAddress.load();
}