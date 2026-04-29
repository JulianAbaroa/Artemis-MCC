#include "pch.h"
#include "Core/Hooks/Domain/Map/Hook_BlamOpenMap.h"
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Domain/Core_System_Domain.h"
#include "Core/Systems/Domain/Map/System_Map.h"
#include "Core/Systems/Domain/Map/System_MapTagGroup.h"
#include "Core/Systems/Domain/Environment/System_Environment.h"
#include "Core/Systems/Domain/Navigation/System_Navigation.h"
#include "Core/Systems/Domain/Interactable/System_Interactable.h"
#include "Core/Systems/Infrastructure/Core_System_Infrastructure.h"
#include "Core/Systems/Infrastructure/Engine/System_Scanner.h"
#include "Core/Systems/Interface/System_Debug.h"
#include "External/minhook/include/MinHook.h"
#include <filesystem>
#include <string>

void Hook_BlamOpenMap::HookedBlamOpenMap(uint64_t param_1, 
	uint64_t param_2, uint64_t mapRelativePath, uint32_t* param_4)
{
	std::string relativePath = reinterpret_cast<const char*>(mapRelativePath);

	m_OriginalFunction(param_1, param_2, mapRelativePath, param_4);

	if (relativePath.contains("campaign") ||
		relativePath.contains("shared")) return;

	char exePath[MAX_PATH];
	GetModuleFileNameA(NULL, exePath, MAX_PATH);

	std::filesystem::path gameRoot =
		std::filesystem::path(exePath).parent_path().
		parent_path().parent_path().parent_path();

	std::filesystem::path fullPath = gameRoot / relativePath;

	bool mapLoaded = g_pSystem->Domain->Map->LoadMap(fullPath.string());
	if (mapLoaded)
	{
		g_pSystem->Domain->MapTagGroup->LoadForMap();
		g_pSystem->Domain->Environment->BuildForMap();
		g_pSystem->Domain->Navigation->BuildForMap();
		g_pSystem->Domain->Interactable->BuildForMap();
	}
}

void Hook_BlamOpenMap::Install()
{
	if (m_IsHookInstalled.load()) return;

	void* functionAddress =
		(void*)g_pSystem->Infrastructure->Scanner->FindPattern(
			Signatures::BlamOpenMap);

	if (!functionAddress)
	{
		g_pSystem->Debug->Log("[BlamOpenMap] ERROR:"
			" Failed to obtain the function address.");

		return;
	}

	m_FunctionAddress.store(functionAddress);
	if (MH_CreateHook(m_FunctionAddress.load(),
		&this->HookedBlamOpenMap,
		reinterpret_cast<LPVOID*>(&m_OriginalFunction)
	) != MH_OK)
	{
		g_pSystem->Debug->Log("[BlamOpenMap] ERROR:"
			" Failed to create the hook.");

		return;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
	{
		g_pSystem->Debug->Log(" [BlamOpenMap] ERROR:"
			" Failed to enable hook.");

		return;
	}

	m_IsHookInstalled.store(true);
	g_pSystem->Debug->Log("[BlamOpenMap] INFO: Hook installed.");
	return;
}

void Hook_BlamOpenMap::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);
	g_pSystem->Debug->Log("[BlamOpenMap] INFO: Hook uninstalled.");
}