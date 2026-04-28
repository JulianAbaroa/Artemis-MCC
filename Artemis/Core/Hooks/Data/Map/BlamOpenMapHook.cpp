#include "pch.h"
#include "Core/Hooks/Data/Map/BlamOpenMapHook.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Domain/CoreDomainSystem.h"
#include "Core/Systems/Domain/Map/MapSystem.h"
#include "Core/Systems/Domain/Map/MapTagGroupSystem.h"
#include "Core/Systems/Domain/Navigation/NavigationSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Engine/ScannerSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include "External/minhook/include/MinHook.h"
#include <filesystem>
#include <string>

void BlamOpenMapHook::HookedBlamOpenMap(uint64_t param_1, 
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
		g_pSystem->Domain->Navigation->BuildForMap();
	}
}

void BlamOpenMapHook::Install()
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

void BlamOpenMapHook::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);
	g_pSystem->Debug->Log("[BlamOpenMap] INFO: Hook uninstalled.");
}