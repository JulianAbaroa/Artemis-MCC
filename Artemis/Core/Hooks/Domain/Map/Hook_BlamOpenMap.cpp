#include "pch.h"

// Header.
#include "Hook_BlamOpenMap.h"

// Types.
#include "Core/Types/Infrastructure/AOB/Signatures.h"

// --- Systems ---

#include "Core/Systems/Domain/Map/System_Map.h"
#include "Core/Systems/Domain/Map/System_MapTagGroup.h"
#include "Core/Systems/Domain/Navigation/System_Navigation.h"
#include "Core/Systems/Domain/Environment/System_Environment.h"
#include "Core/Systems/Domain/Interactable/System_Interactable.h"

#include "Core/Systems/Infrastructure/Engine/Memory/System_AOBScanner.h"

#include "Core/Systems/Interface/Debug/System_Debug.h"

// MinHook.
#include "External/minhook/include/MinHook.h"

#include <filesystem>
#include <string>

void Hook_BlamOpenMap::HookedBlamOpenMap(uint64_t param_1, 
	uint64_t param_2, uint64_t mapRelativePath, uint32_t* param_4)
{
	std::string relativePath = 
		reinterpret_cast<const char*>(mapRelativePath);

	m_OriginalFunction(param_1, param_2, mapRelativePath, param_4);

	if (relativePath.contains("campaign") ||
		relativePath.contains("shared")) return;

	char exePath[MAX_PATH];
	GetModuleFileNameA(NULL, exePath, MAX_PATH);

	std::filesystem::path gameRoot =
		std::filesystem::path(exePath).parent_path().
		parent_path().parent_path().parent_path();

	std::filesystem::path fullPath = gameRoot / relativePath;

	bool mapLoaded = s_Instance->m_Deps.
		System_Map.LoadMap(fullPath.string());

	if (mapLoaded)
	{
		s_Instance->m_Deps.System_MapTagGroup.LoadForMap();
		s_Instance->m_Deps.System_Navigation.BuildForMap();
		s_Instance->m_Deps.System_Environment.BuildForMap();
		s_Instance->m_Deps.System_Interactable.BuildForMap();
	}
}

Hook_BlamOpenMap* Hook_BlamOpenMap::s_Instance = nullptr;

void Hook_BlamOpenMap::Install()
{
	if (m_IsHookInstalled.load()) return;
	s_Instance = this;

	void* functionAddress = (void*)s_Instance->m_Deps.
		System_AOBScanner.FindPattern(Signatures::BlamOpenMap);

	if (!functionAddress)
	{
		s_Instance->m_Deps.System_Debug.Log("[BlamOpenMap] ERROR:"
			" Failed to obtain the function address.");
		return;
	}

	m_FunctionAddress.store(functionAddress);
	if (MH_CreateHook(m_FunctionAddress.load(),
		&this->HookedBlamOpenMap,
		reinterpret_cast<LPVOID*>(&m_OriginalFunction)
	) != MH_OK)
	{
		s_Instance->m_Deps.System_Debug.Log("[BlamOpenMap] ERROR:"
			" Failed to create the hook.");

		return;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
	{
		s_Instance->m_Deps.System_Debug.Log(" [BlamOpenMap] ERROR:"
			" Failed to enable hook.");

		return;
	}

	m_IsHookInstalled.store(true);
	s_Instance->m_Deps.System_Debug.Log("[BlamOpenMap] INFO:"
		" Hook installed.");
	return;
}

void Hook_BlamOpenMap::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);

	s_Instance->m_Deps.System_Debug.Log("[BlamOpenMap] INFO:"
		" Hook uninstalled.");

	s_Instance = nullptr;
}