module;

#include "External/minhook/include/MinHook.h"

module Map.Reader.Hook;
import :OpenMap;

import Platform.Memory.Type;
import std;

namespace
{
	namespace Signature = Platform::Memory::Type::Signature;
}

namespace Map::Reader::Hook
{
	auto __fastcall OpenMapDetour::HookedOpenMap(std::uint64_t param_1,
		std::uint64_t param_2, std::uint64_t mapRelativePath, 
		std::uint32_t* param_4) -> void
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
			s_Instance->m_FileStore.
				SetCampaignFilePath(fullPath.string());
			return;
		}

		if (relativePath.contains("shared"))
		{
			s_Instance->m_FileStore.
				SetSharedFilePath(fullPath.string());
			return;
		}

		std::string mapPath = fullPath.string();

		s_Instance->m_MapLoaderService.LoadMap(mapPath);
	}

	OpenMapDetour* OpenMapDetour::s_Instance = nullptr;

	auto OpenMapDetour::Install() -> void
	{
		if (m_IsHookInstalled.load()) return;
		s_Instance = this;

		void* functionAddress = (void*)s_Instance->m_AOBService.
			FindPattern(Signature::BlamOpenMap);

		if (!functionAddress)
		{
			s_Instance->m_LogsService.Message("[OpenMapDetour] ERROR:"
				" Failed to obtain the function address.");
			return;
		}

		m_FunctionAddress.store(functionAddress);
		if (MH_CreateHook(m_FunctionAddress.load(),
			&this->HookedOpenMap,
			reinterpret_cast<LPVOID*>(&m_OriginalFunction)
		) != MH_OK)
		{
			s_Instance->m_LogsService.Message("[OpenMapDetour] ERROR:"
				" Failed to create the hook.");

			return;
		}
		if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
		{
			s_Instance->m_LogsService.Message(" [OpenMapDetour] ERROR:"
				" Failed to enable hook.");

			return;
		}

		m_IsHookInstalled.store(true);
		s_Instance->m_LogsService.Message("[OpenMapDetour] INFO: Hook installed.");
		return;
	}

	auto OpenMapDetour::Uninstall() -> void
	{
		if (!m_IsHookInstalled.load()) return;

		MH_DisableHook(m_FunctionAddress.load());
		MH_RemoveHook(m_FunctionAddress.load());

		m_IsHookInstalled.store(false);

		s_Instance->m_LogsService.Message("[OpenMapDetour] INFO: Hook uninstalled.");

		s_Instance = nullptr;
	}
}