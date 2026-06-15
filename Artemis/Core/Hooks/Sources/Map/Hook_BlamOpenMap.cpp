#include "pch.h"

#include "Hook_BlamOpenMap.h"

#include "Core/Types/Other/Memory/AOB/Signatures.h"

#include "Core/States/Sources/MapReader/State_MapReader.h"

#include "Core/Systems/Sources/MapReader/System_MapReader.h"
#include "Core/Systems/Other/Memory/AOB/System_AOBScanner.h"
#include "Core/Systems/Other/Logs/System_Logs.h"

#include "External/minhook/include/MinHook.h"

#include <filesystem>
#include <string>

void Hook_BlamOpenMap::HookedBlamOpenMap(uint64_t param_1, 
	uint64_t param_2, uint64_t mapRelativePath, uint32_t* param_4)
{
	m_OriginalFunction(param_1, param_2, mapRelativePath, param_4);

	std::string relativePath = 
		reinterpret_cast<const char*>(mapRelativePath);

	char exePath[MAX_PATH];
	GetModuleFileNameA(NULL, exePath, MAX_PATH);

	std::filesystem::path gameRoot =
		std::filesystem::path(exePath).parent_path().
		parent_path().parent_path().parent_path();

	std::filesystem::path fullPath = gameRoot / relativePath;

	if (relativePath.contains("campaign"))
	{
		s_Instance->m_Deps.State_MapReader.
			SetCampaignFilePath(fullPath.string());
		return;
	}

	if (relativePath.contains("shared"))
	{
		s_Instance->m_Deps.State_MapReader.
			SetSharedFilePath(fullPath.string());
		return;
	}

	std::string mapPath = fullPath.string();

	s_Instance->m_Deps.System_MapReader.LoadMap(mapPath);
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
		s_Instance->m_Deps.System_Logs.Log("[BlamOpenMap] ERROR:"
			" Failed to obtain the function address.");
		return;
	}

	m_FunctionAddress.store(functionAddress);
	if (MH_CreateHook(m_FunctionAddress.load(),
		&this->HookedBlamOpenMap,
		reinterpret_cast<LPVOID*>(&m_OriginalFunction)
	) != MH_OK)
	{
		s_Instance->m_Deps.System_Logs.Log("[BlamOpenMap] ERROR:"
			" Failed to create the hook.");

		return;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
	{
		s_Instance->m_Deps.System_Logs.Log(" [BlamOpenMap] ERROR:"
			" Failed to enable hook.");

		return;
	}

	m_IsHookInstalled.store(true);
	s_Instance->m_Deps.System_Logs.Log("[BlamOpenMap] INFO:"
		" Hook installed.");
	return;
}

void Hook_BlamOpenMap::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);

	s_Instance->m_Deps.System_Logs.Log("[BlamOpenMap] INFO:"
		" Hook uninstalled.");

	s_Instance = nullptr;
}