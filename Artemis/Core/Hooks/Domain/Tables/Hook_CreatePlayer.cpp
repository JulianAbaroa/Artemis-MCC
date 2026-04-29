#include "pch.h"
#include "Core/Hooks/Domain/Tables/Hook_CreatePlayer.h"
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Domain/Core_System_Domain.h"
#include "Core/Systems/Domain/Tables/System_PlayerTable.h"
#include "Core/Systems/Infrastructure/Core_System_Infrastructure.h"
#include "Core/Systems/Infrastructure/Engine/System_Scanner.h"
#include "Core/Systems/Interface/System_Debug.h"
#include "External/minhook/include/MinHook.h"

uint32_t __fastcall Hook_CreatePlayer::HookedCreatePlayer(
	uint32_t playerIndex, uint64_t pPlayerInfo, uint8_t playerFlags)
{
	auto handle = m_OriginalFunction(playerIndex, pPlayerInfo, playerFlags);
	g_pSystem->Domain->PlayerTable->OnPlayerCreated(handle);
	return handle;
}

void Hook_CreatePlayer::Install()
{
	if (m_IsHookInstalled.load()) return;

	void* functionAddress =
		(void*)g_pSystem->Infrastructure->Scanner->FindPattern(
			Signatures::CreatePlayer);

	if (!functionAddress)
	{
		g_pSystem->Debug->Log("[CreatePlayer] ERROR:"
			" Failed to obtain the function address.");

		return;
	}

	m_FunctionAddress.store(functionAddress);
	if (MH_CreateHook(m_FunctionAddress.load(),
		&this->HookedCreatePlayer,
		reinterpret_cast<LPVOID*>(&m_OriginalFunction)
	) != MH_OK)
	{
		g_pSystem->Debug->Log("[CreatePlayer] ERROR:"
			" Failed to create the hook.");

		return;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
	{
		g_pSystem->Debug->Log(" [CreatePlayer] ERROR:"
			" Failed to enable hook.");

		return;
	}

	m_IsHookInstalled.store(true);
	g_pSystem->Debug->Log("[CreatePlayer] INFO: Hook installed.");
	return;
}

void Hook_CreatePlayer::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);
	g_pSystem->Debug->Log("[CreatePlayer] INFO: Hook uninstalled.");
}