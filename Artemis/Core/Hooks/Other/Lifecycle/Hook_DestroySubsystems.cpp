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

	auto& deps = s_Instance->m_Deps;
	auto& lifecycle = deps.State_Lifecycle;
	auto& logs = deps.System_Logs;

	lifecycle.SetStatus({ Status::TearingDown });
	if (!lifecycle.WaitForLoadEnd(MilliSeconds(1000)))
	{
		logs.Log("[DestroySubsystems] WARNING:"
			" Artemis resources load did not finish within timeout.");
	}
	if (!lifecycle.WaitForTickEnd(MilliSeconds(10)))
	{ 
		logs.Log("[DestroySubsystems] WARNING:"
			" Artemis tick did not finish within timeout.");
	}

	// --- Layer 0: Sources ---
	deps.Hook_BlamOpenMap.Uninstall();
	deps.Hook_CreateObject.Uninstall();
	deps.Hook_ReleaseObject.Uninstall();
	deps.Hook_InitRootNode.Uninstall();
	deps.Hook_CreatePlayer.Uninstall();
	deps.Hook_SimulationTicks.Uninstall();
	
	deps.System_MapReader.Cleanup();
	deps.System_TagGroup.Cleanup();
	deps.System_ObjectTable.Cleanup();
	deps.System_PlayerTable.Cleanup();
	deps.System_InteractionTable.Cleanup();
	deps.System_WorldBuilder.Cleanup();
	deps.System_StatsBuilder.Cleanup();
	deps.System_VitalityBuilder.Cleanup();

	// --- Layer 1: Structure ---
	deps.System_Classifier.Cleanup();
	deps.System_ObjectGraph.Cleanup();
	deps.System_PlayerGraph.Cleanup();

	// --- Layer 2: Environment ---
	deps.System_Collidables.Cleanup();
	deps.System_Fixtures.Cleanup();
	deps.System_Vitality.Cleanup();

	// --- Layer 3: Egocentric ---
	deps.System_Self.Cleanup();
	deps.System_Affordances.Cleanup();

	// Other.
	deps.System_MapRenderer.ReleaseMapMesh();

	deps.UI_ObjectTable.Cleanup();
	deps.UI_PlayerTable.Cleanup();

	m_OriginalFunction();

	lifecycle.SetStatus({ Status::Destroyed });

	logs.Log("[DestroySubsystems] INFO: Game engine destroyed.");

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