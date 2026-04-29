#include "pch.h"
#include "Core/Hooks/Core_Hook.h"
#include "Core/Hooks/Domain/Core_Hook_Domain.h"
#include "Core/Hooks/Domain/Tables/Hook_PlayerTable.h"
#include "Core/States/Core_State.h"
#include "Core/States/Domain/Core_State_Domain.h"
#include "Core/States/Domain/Tables/State_PlayerTable.h"
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Domain/Tables/System_PlayerTable.h"
#include "Core/Systems/Infrastructure/Core_System_Infrastructure.h"
#include "Core/Systems/Infrastructure/Engine/System_Format.h"
#include "Core/Systems/Interface/System_Debug.h"
#include <vector>

void System_PlayerTable::OnPlayerCreated(uint32_t handle)
{
	uintptr_t tableBase = g_pState->Domain->PlayerTable->GetPlayerTableBase();
	if (tableBase == 0) return;

	uint32_t newIndex = handle & 0xFFFF;

	g_pState->Domain->PlayerTable->RemovePlayerIf([newIndex](uint32_t oldHandle, const LivePlayer& p) {
		return (oldHandle & 0xFFFF) == newIndex;
	});

	uint32_t index = handle & 0xFFFF;
	auto* header = reinterpret_cast<const PlayerHeader*>(
		tableBase + index * sizeof(PlayerHeader));

	g_pState->Domain->PlayerTable->AddPlayer(
		handle, this->BuildLivePlayer(handle, header));
}

void System_PlayerTable::OnPlayerDestroyed(uint32_t handle)
{
	auto deletedPlayer = g_pState->Domain->PlayerTable->RemovePlayer(handle);

	if (!deletedPlayer.has_value())
	{
		g_pSystem->Debug->Log("[PlayerTableSystem] WARNING:"
			" OnPlayerDestroyed called for unknown handle 0x%X.", handle);
	}
}

void System_PlayerTable::FindPlayerTableBase()
{
	uintptr_t tableBase = g_pState->Domain->PlayerTable->GetPlayerTableBase();
	if (tableBase == 0)
	{
		tableBase = g_pHook->Domain->PlayerTable->GetPlayerTable();
		if (!tableBase)
		{
			g_pSystem->Debug->Log("[PlayerTableSystem] ERROR:"
				" PlayerTableBase invalid.");

			return;
		}

		g_pSystem->Debug->Log("[PlayerTableSystem] INFO: PlayerTable: 0x%llX", tableBase);
		g_pState->Domain->PlayerTable->SetPlayerTableBase(tableBase);
	}
}

void System_PlayerTable::UpdatePlayerTable()
{
	this->UpdatePlayerData();
}

void System_PlayerTable::Cleanup()
{
	g_pState->Domain->PlayerTable->Cleanup();
	g_pSystem->Debug->Log("[PlayerTableSystem] INFO: Cleanup completed.");
}


LivePlayer System_PlayerTable::BuildLivePlayer(uint32_t handle, const PlayerHeader* header)
{
	LivePlayer player{};
	// TODO: these can change in real-time?
	player.Handle = handle;
	player.Address = reinterpret_cast<uintptr_t>(header);

	player.ConnectedFlag = header->ConnectedFlag;

	player.Gamertag = this->WideToUtf8(header->Gamertag, 32);
	player.Tag = this->WideToUtf8(header->Tag, 16);

	player.PrimaryWeaponHandle = header->PrimaryWeaponHandle;
	player.SecondaryWeaponHandle = header->SecondaryWeaponHandle;
	player.ObjectiveHandle = header->ObjectiveHandle;

	std::memcpy(player.WeaponPosition, header->WeaponPosition, 
		sizeof(player.WeaponPosition));

	std::memcpy(player.WeaponForward, header->WeaponForward, 
		sizeof(player.WeaponForward));

	std::memcpy(player.AimOffset, header->AimOffset, 
		sizeof(player.AimOffset));

	return player;
}

void System_PlayerTable::UpdatePlayerData()
{
	uintptr_t tableBase = g_pState->Domain->PlayerTable->GetPlayerTableBase();
	if (tableBase == 0) return;

	std::vector<uint32_t> handlesToRemove;

	g_pState->Domain->PlayerTable->UpdatePlayers(
		[tableBase, &handlesToRemove](uint32_t handle, LivePlayer& player) {
			uint32_t index = handle & 0xFFFF;
			auto* header = reinterpret_cast<const PlayerHeader*>(
				tableBase + index * sizeof(PlayerHeader));

			uint32_t handleInMemory = (header->Handle << 16) | index;

			if (handleInMemory != handle)
			{
				handlesToRemove.push_back(handle);
				return;
			}

			player.ConnectedFlag = header->ConnectedFlag;

			player.PrimaryWeaponHandle = header->PrimaryWeaponHandle;
			player.SecondaryWeaponHandle = header->SecondaryWeaponHandle;
			player.ObjectiveHandle = header->ObjectiveHandle;

			player.AliveBipedHandle = header->AliveBipedHandle;
			player.DeadBipedHandle = header->DeadBipedHandle;
			player.CurrentBipedHandle = header->CurrentBipedHandle;

			player.IsAlive =
				player.AliveBipedHandle == 0xFFFFFFFF ? false : true;

			std::memcpy(player.WeaponPosition, header->WeaponPosition, sizeof(player.WeaponPosition));
			std::memcpy(player.WeaponForward, header->WeaponForward, sizeof(player.WeaponForward));
			std::memcpy(player.AimOffset, header->AimOffset, sizeof(player.AimOffset));
		}
	);

	for (uint32_t handle : handlesToRemove)
	{
		g_pState->Domain->PlayerTable->RemovePlayer(handle);
	}
}


std::string System_PlayerTable::WideToUtf8(const wchar_t* src, size_t maxLen)
{
	if (!src || maxLen == 0) return std::string();

	int wideLen = (int)wcsnlen(src, maxLen);
	if (wideLen == 0) return std::string();

	int utf8Len = WideCharToMultiByte(CP_UTF8, 0, src, wideLen, NULL, 0, NULL, NULL);

	std::string res(utf8Len, 0);

	WideCharToMultiByte(CP_UTF8, 0, src, wideLen, &res[0], utf8Len, NULL, NULL);

	return res;
}