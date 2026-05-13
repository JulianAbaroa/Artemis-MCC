#include "pch.h"

// Head.
#include "Hook_EngineInitialize.h"

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

#include "Core/Systems/Domain/Object/System_ObjectTable.h"
#include "Core/Systems/Domain/Player/System_PlayerTable.h"
#include "Core/Systems/Domain/Interaction/System_InteractionTable.h"

#include "Core/Systems/Infrastructure/Engine/Memory/System_AOBScanner.h"

#include "Core/Systems/Interface/System_Debug.h"

// MinHook.
#include "External/minhook/include/MinHook.h"

void __fastcall Hook_EngineInitialize::HookedEngineInitialize(void)
{
	m_OriginalFunction();

	// --- Hooks ---
	g_pHook->Domain->BlamOpenMap->Install();
	g_pHook->Domain->CreateGO->Install();
	g_pHook->Domain->ReleaseGO->Install();
	g_pHook->Domain->CreatePlayer->Install();

	g_pHook->Infrastructure->GetButtonState->Install();

	// --- Systems ---
	g_pSystem->Domain->ObjectTable->FindObjectTableBase();
	g_pSystem->Domain->PlayerTable->FindPlayerTableBase();
	g_pSystem->Domain->InteractionTable->FindInteractionTableBase();
	
	g_pState->Infrastructure->Lifecycle->SetEngineStatus(
		{ EngineStatus::Running });

	auto& debug = *g_pSystem->Debug;
	debug.Log("[EngineInitialize] INFO: Game engine initialized.");
}

bool Hook_EngineInitialize::Install()
{
	if (m_IsHookInstalled.load()) return true;

	auto& debug = *g_pSystem->Debug;
	auto& aobScanner = *g_pSystem->Infrastructure->AOBScanner;
	
	void* functionAddress = 
		(void*)aobScanner.FindPattern(Signatures::EngineInitialize);
	
	if (!functionAddress) return false;

	m_FunctionAddress.store(functionAddress);
	MH_RemoveHook(m_FunctionAddress.load());

	if (MH_CreateHook(
			m_FunctionAddress.load(), 
		&this->HookedEngineInitialize, 
		reinterpret_cast<LPVOID*>(&m_OriginalFunction)) 
		!= MH_OK)
	{
		debug.Log("[EngineInitialize] ERROR: Failed to create the hook.");
		return false;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK) 
	{
		debug.Log("[EngineInitialize] ERROR: Failed to enable the hook.");
		return false;
	}

	m_IsHookInstalled.store(true);
	debug.Log("[EngineInitialize] INFO: Hook installed.");
	return true;
}

void Hook_EngineInitialize::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);

	auto& debug = *g_pSystem->Debug;
	debug.Log("[EngineInitialize] INFO: Hook uninstalled.");
}

void* Hook_EngineInitialize::GetFunctionAddress()
{
	return m_FunctionAddress.load();
}