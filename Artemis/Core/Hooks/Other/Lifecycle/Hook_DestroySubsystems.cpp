#include "pch.h"

#include "Hook_DestroySubsystems.h"

#include "Core/Types/Other/Memory/AOB/Signatures.h"

#include "Core/Hooks/Sources/Map/Hook_BlamOpenMap.h"
#include "Core/Hooks/Sources/Object/Hook_CreateObject.h"
#include "Core/Hooks/Sources/Object/Hook_ReleaseObject.h"
#include "Core/Hooks/Sources/Object/BoneMatrix/Hook_InitRootNode.h"
#include "Core/Hooks/Sources/Player/Hook_CreatePlayer.h"
#include "Core/Hooks/Tick/Hook_SimulationTicks.h"
#include "Core/Hooks/Other/Input/Hook_GetButtonState.h"

#include "Core/States/Other/Lifecycle/State_Lifecycle.h"

#include "Core/Systems/Sources/MapReader/System_MapReader.h"
#include "Core/Systems/Sources/MapReader/TagGroup/System_TagGroupReader.h"
#include "Core/Systems/Sources/MapReader/Geometry/System_GeometryReader.h"
#include "Core/Systems/Sources/Tables/Object/System_ObjectTable.h"
#include "Core/Systems/Sources/Tables/Player/System_PlayerTable.h"
#include "Core/Systems/Sources/Tables/Interaction/System_InteractionTable.h"
#include "Core/Systems/Structure/Classifier/System_Classifier.h"
#include "Core/Systems/Structure/Graph/Object/System_ObjectGraph.h"
#include "Core/Systems/Structure/Graph/Player/System_PlayerGraph.h"
#include "Core/Systems/Sources/Static/World/System_WorldBuilder.h"
#include "Core/Systems/Sources/Static/Stats/System_StatsBuilder.h"
#include "Core/Systems/Sources/Static/Vitality/System_VitalityBuilder.h"
#include "Core/Systems/Environment/Collidables/System_Collidables.h"
#include "Core/Systems/Environment/Vitality/System_Vitality.h"
#include "Core/Systems/Environment/Fixtures/System_Fixtures.h"
#include "Core/Systems/Egocentric/Affordances/System_Affordances.h"	
#include "Core/Systems/Egocentric/Self/System_Self.h"
#include "Core/Systems/Other/Memory/AOB/System_AOBScanner.h"
#include "Core/Systems/Other/Render/Map/System_MapRenderer.h"
#include "Core/Systems/Other/Logs/System_Logs.h"

#include "Core/UI/ObjectTable/UI_ObjectTable.h"
#include "Core/UI/PlayerTable/UI_PlayerTable.h"

#include "External/minhook/include/MinHook.h"

void __fastcall Hook_DestroySubsystems::HookedDestroySubsystems(void)
{
	s_InProgress.store(true, std::memory_order_release);

	s_Instance->m_Deps.State_Lifecycle.SetTearingDown(true);
	s_Instance->m_Deps.State_Lifecycle.WakeTickWaiters();

	bool idle = s_Instance->m_Deps.State_Lifecycle.WaitForAIIdle(
		std::chrono::milliseconds(50));
	if (!idle)
	{ 
		s_Instance->m_Deps.System_Logs.Log("[DestroySubsystems] WARN:"
			" AI sweep did not finish within timeout; proceeding anyway.");
	}

	s_Instance->m_Deps.Hook_BlamOpenMap.Uninstall();
	s_Instance->m_Deps.Hook_CreateObject.Uninstall();
	s_Instance->m_Deps.Hook_ReleaseObject.Uninstall();
	s_Instance->m_Deps.Hook_InitRootNode.Uninstall();
	s_Instance->m_Deps.Hook_CreatePlayer.Uninstall();
	s_Instance->m_Deps.Hook_SimulationTicks.Uninstall();

	s_Instance->m_Deps.System_MapReader.Cleanup();
	s_Instance->m_Deps.System_TagGroup.Cleanup();
	s_Instance->m_Deps.System_ObjectTable.Cleanup();
	s_Instance->m_Deps.System_PlayerTable.Cleanup();
	s_Instance->m_Deps.System_InteractionTable.Cleanup();
	s_Instance->m_Deps.System_Classifier.Cleanup();
	s_Instance->m_Deps.System_ObjectGraph.Cleanup();
	s_Instance->m_Deps.System_PlayerGraph.Cleanup();
	s_Instance->m_Deps.System_Collidables.Cleanup();
	s_Instance->m_Deps.System_WorldBuilder.Cleanup();
	s_Instance->m_Deps.System_StatsBuilder.Cleanup();
	s_Instance->m_Deps.System_VitalityBuilder.Cleanup();
	s_Instance->m_Deps.System_Vitality.Cleanup();
	s_Instance->m_Deps.System_Self.Cleanup();
	s_Instance->m_Deps.System_Fixtures.Cleanup();
	s_Instance->m_Deps.System_Affordances.Cleanup();
	s_Instance->m_Deps.System_MapRenderer.ReleaseMapMesh();

	s_Instance->m_Deps.UI_ObjectTable.Cleanup();
	s_Instance->m_Deps.UI_PlayerTable.Cleanup();

	m_OriginalFunction();

	s_Instance->m_Deps.State_Lifecycle.SetEngineStatus({ EngineStatus::Destroyed });
	s_Instance->m_Deps.State_Lifecycle.SetTearingDown(false);

	s_Instance->m_Deps.System_Logs.Log("[DestroySubsystems] INFO:"
		" Game engine destroyed.");

	s_InProgress.store(false, std::memory_order_release);
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

	if (s_Instance)
	{
		s_Instance->m_Deps.System_Logs.Log("[DestroySubsystems] INFO:"
			" Hook uninstalled.");
	}
}

void* Hook_DestroySubsystems::GetFunctionAddress()
{
	return m_FunctionAddress.load();
}