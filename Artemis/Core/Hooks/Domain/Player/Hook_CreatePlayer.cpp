#include "pch.h"

// Header.
#include "Hook_CreatePlayer.h"

// Types.
#include "Core/Types/Infrastructure/AOB/Signatures.h"

// --- Systems ---

#include "Core/Systems/Domain/Player/System_PlayerTable.h"

#include "Core/Systems/Infrastructure/Engine/Memory/System_AOBScanner.h"

#include "Core/Systems/Interface/Debug/System_Debug.h"

// MinHook.
#include "External/minhook/include/MinHook.h"

uint32_t __fastcall Hook_CreatePlayer::HookedCreatePlayer(
	uint32_t playerIndex, uint64_t pPlayerInfo, uint8_t playerFlags)
{
	auto handle = m_OriginalFunction(playerIndex, pPlayerInfo, playerFlags);

	s_Instance->m_Deps.System_PlayerTable.OnPlayerCreated(handle);

	return handle;
}

Hook_CreatePlayer* Hook_CreatePlayer::s_Instance = nullptr;

void Hook_CreatePlayer::Install()
{
	if (m_IsHookInstalled.load()) return;
	s_Instance = this;

	void* functionAddress = (void*)s_Instance->m_Deps.
		System_AOBScanner.FindPattern(Signatures::CreatePlayer);

	if (!functionAddress)
	{
		s_Instance->m_Deps.System_Debug.Log("[CreatePlayer] ERROR:"
			" Failed to obtain the function address.");
		return;
	}

	m_FunctionAddress.store(functionAddress);
	if (MH_CreateHook(m_FunctionAddress.load(),
		&this->HookedCreatePlayer,
		reinterpret_cast<LPVOID*>(&m_OriginalFunction)
	) != MH_OK)
	{
		s_Instance->m_Deps.System_Debug.Log("[CreatePlayer] ERROR:"
			" Failed to create the hook.");
		return;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
	{
		s_Instance->m_Deps.System_Debug.Log(" [CreatePlayer] ERROR:"
			" Failed to enable hook.");
		return;
	}

	m_IsHookInstalled.store(true);
	s_Instance->m_Deps.System_Debug.Log("[CreatePlayer] INFO:"
		" Hook installed.");
	return;
}

void Hook_CreatePlayer::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);

	s_Instance->m_Deps.System_Debug.Log("[CreatePlayer] INFO:"
		" Hook uninstalled.");

	s_Instance = nullptr;
}