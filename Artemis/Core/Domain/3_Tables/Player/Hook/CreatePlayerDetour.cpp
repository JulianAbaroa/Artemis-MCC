module;

#include "External/minhook/include/MinHook.h"

module Tables.Player.Hook;
import :CreatePlayer;

import Platform.Memory.Type;

namespace
{
	namespace Signature = Platform::Memory::Type::Signature;
}

namespace Tables::Player::Hook
{
	auto __fastcall CreatePlayerDetour::HookedCreatePlayer(std::uint32_t playerIndex,
		std::uint64_t pPlayerInfo, std::uint8_t playerFlags) -> std::uint32_t
	{
		auto handle = m_OriginalFunction(playerIndex, pPlayerInfo, playerFlags);
		s_Instance->m_PlayerService.OnPlayerCreated(handle);
		return handle;
	}

	CreatePlayerDetour* CreatePlayerDetour::s_Instance = nullptr;

	void CreatePlayerDetour::Install()
	{
		if (m_IsHookInstalled.load()) return;
		s_Instance = this;

		void* functionAddress = (void*)s_Instance->m_AOBService.
			FindPattern(Signature::CreatePlayer);

		if (!functionAddress)
		{
			s_Instance->m_LogsService.Message("[CreatePlayerDetour] ERROR:"
				" Failed to obtain the function address.");
			return;
		}

		m_FunctionAddress.store(functionAddress);
		if (MH_CreateHook(m_FunctionAddress.load(),
			&this->HookedCreatePlayer,
			reinterpret_cast<LPVOID*>(&m_OriginalFunction)
		) != MH_OK)
		{
			s_Instance->m_LogsService.Message("[CreatePlayerDetour] ERROR:"
				" Failed to create the hook.");
			return;
		}
		if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
		{
			s_Instance->m_LogsService.Message(" [CreatePlayerDetour] ERROR:"
				" Failed to enable hook.");
			return;
		}

		m_IsHookInstalled.store(true);
		s_Instance->m_LogsService.Message("[CreatePlayerDetour] INFO:"
			" Hook installed.");
		return;
	}

	void CreatePlayerDetour::Uninstall()
	{
		if (!m_IsHookInstalled.load()) return;

		MH_DisableHook(m_FunctionAddress.load());
		MH_RemoveHook(m_FunctionAddress.load());

		m_IsHookInstalled.store(false);

		s_Instance->m_LogsService.Message("[CreatePlayerDetour] INFO:"
			" Hook uninstalled.");

		s_Instance = nullptr;
	}
}