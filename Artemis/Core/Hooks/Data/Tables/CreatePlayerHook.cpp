#include "pch.h"
#include "Core/Hooks/Data/Tables/CreatePlayerHook.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Domain/CoreDomainSystem.h"
#include "Core/Systems/Domain/Tables/PlayerTableSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Engine/ScannerSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include "External/minhook/include/MinHook.h"

uint32_t __fastcall CreatePlayerHook::HookedCreatePlayer(
	uint32_t playerIndex, uint64_t pPlayerInfo, uint8_t playerFlags)
{
	auto handle = m_OriginalFunction(playerIndex, pPlayerInfo, playerFlags);
	g_pSystem->Domain->PlayerTable->OnPlayerCreated(handle);
	return handle;
}

void CreatePlayerHook::Install()
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

void CreatePlayerHook::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);
	g_pSystem->Debug->Log("[CreatePlayer] INFO: Hook uninstalled.");
}