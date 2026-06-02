#include "pch.h"

#include "Hook_DestroySubsystems.h"

#include "Core/Types/AOB/Signatures.h"

#include "Core/Hooks/Map/Hook_BlamOpenMap.h"
#include "Core/Hooks/Object/Hook_CreateObject.h"
#include "Core/Hooks/Object/Hook_ReleaseObject.h"
#include "Core/Hooks/Player/Hook_CreatePlayer.h"
#include "Core/Hooks/Input/Hook_GetButtonState.h"

#include "Core/States/Lifecycle/State_Lifecycle.h"

#include "Core/Systems/MapReader/System_MapReader.h"
#include "Core/Systems/MapReader/TagGroup/System_TagGroupReader.h"
#include "Core/Systems/Tables/Object/System_ObjectTable.h"
#include "Core/Systems/Tables/Player/System_PlayerTable.h"
#include "Core/Systems/Tables/Interaction/System_InteractionTable.h"
#include "Core/Systems/Filtered/Classifier/System_Classifier.h"
#include "Core/Systems/Filtered/Graph/Object/System_ObjectGraph.h"
#include "Core/Systems/Filtered/Graph/Player/System_PlayerGraph.h"
#include "Core/Systems/Domains/Navigation/System_Navigation.h"
#include "Core/Systems/Domains/Environment/System_Environment.h"
#include "Core/Systems/Domains/Interactable/System_Interactable.h"
#include "Core/Systems/Memory/AOBScanner/System_AOBScanner.h"
#include "Core/Systems/Logs/System_Logs.h"

#include "Core/UI/ObjectTable/UI_ObjectTable.h"
#include "Core/UI/PlayerTable/UI_PlayerTable.h"
#include "Core/UI/Interactable/UI_Interactable.h"
#include "Core/UI/Map/UI_Map.h"

#include "External/minhook/include/MinHook.h"

void __fastcall Hook_DestroySubsystems::HookedDestroySubsystems(void)
{
	s_Instance->m_Deps.Hook_BlamOpenMap.Uninstall();
	s_Instance->m_Deps.Hook_CreateObject.Uninstall();
	s_Instance->m_Deps.Hook_ReleaseObject.Uninstall();
	s_Instance->m_Deps.Hook_CreatePlayer.Uninstall();

	s_Instance->m_Deps.System_MapReader.Cleanup();
	s_Instance->m_Deps.System_TagGroup.Cleanup();
	s_Instance->m_Deps.System_ObjectTable.Cleanup();
	s_Instance->m_Deps.System_PlayerTable.Cleanup();
	s_Instance->m_Deps.System_InteractionTable.Cleanup();
	s_Instance->m_Deps.System_Classifier.Cleanup();
	s_Instance->m_Deps.System_ObjectGraph.Cleanup();
	s_Instance->m_Deps.System_PlayerGraph.Cleanup();
	s_Instance->m_Deps.System_Navigation.Cleanup();
	s_Instance->m_Deps.System_Environment.Cleanup();
	s_Instance->m_Deps.System_Interactable.Cleanup();

	s_Instance->m_Deps.UI_ObjectTable.Cleanup();
	s_Instance->m_Deps.UI_PlayerTable.Cleanup();
	s_Instance->m_Deps.UI_Map.Cleanup();

	s_Instance->m_Deps.State_Lifecycle.SetEngineStatus(
		{ EngineStatus::Destroyed });

	m_OriginalFunction();

	s_Instance->m_Deps.System_Logs.Log("[DestroySubsystems] INFO:"
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
		s_Instance->m_Deps.System_Logs.Log("[DestroySubsystems] ERROR:"
			" Failed to create the hook.");
		return false;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK) 
	{
		s_Instance->m_Deps.System_Logs.Log("[DestroySubsystems] ERROR:"
			" Failed to enable the hook.");
		return false;
	}

	m_IsHookInstalled.store(true);
	s_Instance->m_Deps.System_Logs.Log("[DestroySubsystems] INFO:"
		" Hook installed.");
	return true;
}

void Hook_DestroySubsystems::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);

	s_Instance->m_Deps.System_Logs.Log("[DestroySubsystems] INFO:"
		" Hook uninstalled.");

	s_Instance = nullptr;
}

void* Hook_DestroySubsystems::GetFunctionAddress()
{
	return m_FunctionAddress.load();
}