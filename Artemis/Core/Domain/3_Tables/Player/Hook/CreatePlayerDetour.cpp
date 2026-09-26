module;

#include "External/minhook/include/MinHook.h"

module Tables.Player.Hook;
import :CreatePlayer;

import Platform.Memory.Type;
import Platform.Hook.Common;

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

        void* functionAddress = (void*)m_AOBService.FindPattern(Signature::CreatePlayer);
        m_FunctionAddress.store(functionAddress);

        if (!Platform::Hook::Common::InstallDetour(functionAddress,
            reinterpret_cast<void*>(&HookedCreatePlayer),
            reinterpret_cast<void**>(&m_OriginalFunction),
            "[CreatePlayerDetour]", m_LogsService))
        {
            return;
        }

        m_IsHookInstalled.store(true);
    }

    void CreatePlayerDetour::Uninstall()
    {
        if (!m_IsHookInstalled.load()) return;

        Platform::Hook::Common::UninstallDetour(m_FunctionAddress.load(),
            "[CreatePlayerDetour]", m_LogsService);

        m_IsHookInstalled.store(false);
        s_Instance = nullptr;
    }
}