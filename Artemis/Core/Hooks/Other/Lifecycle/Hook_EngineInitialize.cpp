#include "pch.h"

#include "Hook_EngineInitialize.h"

#include "Core/Types/Other/Memory/AOB/Signatures.h"

#include "Core/Hooks/Sources/Map/Hook_BlamOpenMap.h"
#include "Core/Hooks/Sources/Object/Hook_ObjectTable.h"
#include "Core/Hooks/Sources/Object/Hook_CreateObject.h"
#include "Core/Hooks/Sources/Object/Hook_ReleaseObject.h"
#include "Core/Hooks/Sources/Object/BoneMatrix/Hook_InitRootNode.h"
#include "Core/Hooks/Sources/Player/Hook_PlayerTable.h"
#include "Core/Hooks/Sources/Player/Hook_CreatePlayer.h"
#include "Core/Hooks/Sources/Interaction/Hook_InteractionTable.h"
#include "Core/Hooks/Tick/Hook_SimulationTicks.h"
#include "Core/Hooks/Other/Input/Hook_GetButtonState.h"

#include "Core/States/Other/Lifecycle/State_Lifecycle.h"

#include "Core/Systems/Other/Memory/AOB/System_AOBScanner.h"
#include "Core/Systems/Other/Logs/System_Logs.h"

#include "External/minhook/include/MinHook.h"

void __fastcall Hook_EngineInitialize::HookedEngineInitialize(void)
{
	m_OriginalFunction();

	s_Instance->m_Deps.Hook_BlamOpenMap.Install();
	s_Instance->m_Deps.Hook_CreateObject.Install();
	s_Instance->m_Deps.Hook_ReleaseObject.Install();
	s_Instance->m_Deps.Hook_InitRootNode.Install();
	s_Instance->m_Deps.Hook_CreatePlayer.Install();
	s_Instance->m_Deps.Hook_SimulationTicks.Install();

	s_Instance->m_Deps.Hook_ObjectTable.FindAndStoreTableBase();
	s_Instance->m_Deps.Hook_PlayerTable.FindAndStoreTableBase();
	s_Instance->m_Deps.Hook_InteractionTable.FindAndStoreTableBase();
	
	s_Instance->m_Deps.State_Lifecycle.SetEngineStatus(
		{ EngineStatus::Running });

	s_Instance->m_Deps.System_Logs.Log("[EngineInitialize] INFO:"
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
		s_Instance->m_Deps.System_Logs.Log("[EngineInitialize] ERROR:"
			" Failed to create the hook.");
		return false;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK) 
	{
		s_Instance->m_Deps.System_Logs.Log("[EngineInitialize] ERROR:"
			" Failed to enable the hook.");
		return false;
	}

	m_IsHookInstalled.store(true);
	s_Instance->m_Deps.System_Logs.Log("[EngineInitialize] INFO:"
		" Hook installed.");
	return true;
}

void Hook_EngineInitialize::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);

	s_Instance->m_Deps.System_Logs.Log("[EngineInitialize] INFO:"
		" Hook uninstalled.");

	s_Instance = nullptr;
}

void* Hook_EngineInitialize::GetFunctionAddress()
{
	return m_FunctionAddress.load();
}