#include "pch.h"

// Header.
#include "Hook_BlamOpenMap.h"

// --- Systems ---
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Domain/Core_System_Domain.h"
#include "Core/Systems/Infrastructure/Core_System_Infrastructure.h"

#include "Core/Systems/Domain/Map/System_Map.h"
#include "Core/Systems/Domain/Map/System_MapTagGroup.h"
#include "Core/Systems/Domain/Navigation/System_Navigation.h"
#include "Core/Systems/Domain/Environment/System_Environment.h"
#include "Core/Systems/Domain/Interactable/System_Interactable.h"

#include "Core/Systems/Infrastructure/Engine/Memory/System_AOBScanner.h"

#include "Core/Systems/Interface/System_Debug.h"

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

	auto& map = *g_pSystem->Domain->Map;
	bool mapLoaded = map.LoadMap(fullPath.string());
	if (mapLoaded)
	{
		auto& mapTagGroup = *g_pSystem->Domain->MapTagGroup;
		mapTagGroup.LoadForMap();

		auto& navigation = *g_pSystem->Domain->Navigation;
		navigation.BuildForMap();

		auto& environment = *g_pSystem->Domain->Environment;
		environment.BuildForMap();

		auto& interactable = *g_pSystem->Domain->Interactable;
		interactable.BuildForMap();
	}
}

void Hook_BlamOpenMap::Install()
{
	if (m_IsHookInstalled.load()) return;

	auto& debug = *g_pSystem->Debug;
	auto& aobScanner = *g_pSystem->Infrastructure->AOBScanner;
	
	void* functionAddress = 
		(void*)aobScanner.FindPattern(Signatures::BlamOpenMap);

	if (!functionAddress)
	{
		debug.Log("[BlamOpenMap] ERROR: Failed to obtain"
			" the function address.");
		return;
	}

	m_FunctionAddress.store(functionAddress);
	if (MH_CreateHook(m_FunctionAddress.load(),
		&this->HookedBlamOpenMap,
		reinterpret_cast<LPVOID*>(&m_OriginalFunction)
	) != MH_OK)
	{
		debug.Log("[BlamOpenMap] ERROR:"
			" Failed to create the hook.");

		return;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
	{
		debug.Log(" [BlamOpenMap] ERROR:"
			" Failed to enable hook.");

		return;
	}

	m_IsHookInstalled.store(true);
	debug.Log("[BlamOpenMap] INFO: Hook installed.");
	return;
}

void Hook_BlamOpenMap::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);

	auto& debug = *g_pSystem->Debug;
	debug.Log("[BlamOpenMap] INFO: Hook uninstalled.");
}