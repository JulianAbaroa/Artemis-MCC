#include "pch.h"
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
#include "Core/Hooks/Infrastructure/Lifecycle/Hook_EngineInitialize.h"

#include "Core/States/Core_State.h"
#include "Core/States/Infrastructure/Core_State_Infrastructure.h"
#include "Core/States/Infrastructure/Engine/State_Lifecycle.h"

#include "Core/Systems/Core_System.h"
#include "Core/Systems/Domain/Core_System_Domain.h"
#include "Core/Systems/Domain/Tables/System_ObjectTable.h"
#include "Core/Systems/Domain/Tables/System_PlayerTable.h"
#include "Core/Systems/Domain/Tables/System_InteractionTable.h"
#include "Core/Systems/Infrastructure/Core_System_Infrastructure.h"
#include "Core/Systems/Infrastructure/Engine/System_Scanner.h"
#include "Core/Systems/Interface/System_Debug.h"
#include "External/minhook/include/MinHook.h"

void __fastcall Hook_EngineInitialize::HookedEngineInitialize(void)
{
	m_OriginalFunction();

	// Hooks: Map.
	g_pHook->Domain->BlamOpenMap->Install();

	// Hooks: Tables
	g_pHook->Domain->CreateGO->Install();
	g_pHook->Domain->ReleaseGO->Install();
	g_pHook->Domain->CreatePlayer->Install();

	// Other.
	g_pHook->Domain->BuildGameEvent->Install();
	g_pHook->Infrastructure->GetButtonState->Install();

	// Systems: Tables.
	g_pSystem->Domain->ObjectTable->FindObjectTableBase();
	g_pSystem->Domain->PlayerTable->FindPlayerTableBase();
	g_pSystem->Domain->InteractionTable->FindInteractionTableBase();
	
	g_pState->Infrastructure->Lifecycle->SetEngineStatus(
		{ EngineStatus::Running });

	g_pSystem->Debug->Log("[EngineInitialize] INFO: Game engine initialized.");
}

bool Hook_EngineInitialize::Install(bool silent)
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

void Hook_EngineInitialize::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);
	g_pSystem->Debug->Log("[EngineInitialize] INFO: Hook uninstalled.");
}

void* Hook_EngineInitialize::GetFunctionAddress()
{
	return m_FunctionAddress.load();
}