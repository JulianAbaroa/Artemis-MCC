#include "pch.h"
#include "Core/Hooks/CoreHook.h"
#include "Core/Hooks/Memory/CoreMemoryHook.h"
#include "Core/Hooks/Memory/PlayerTableHook.h"
#include "Core/States/CoreState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Domain/Tables/PlayerTableState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Domain/Tables/PlayerTableSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Engine/FormatSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include <vector>

void PlayerTableSystem::OnPlayerCreated(uint32_t handle)
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

void PlayerTableSystem::OnPlayerDestroyed(uint32_t handle)
{
	auto deletedPlayer = g_pState->Domain->PlayerTable->RemovePlayer(handle);

	if (!deletedPlayer.has_value())
	{
		g_pSystem->Debug->Log("[PlayerTableSystem] WARNING:"
			" OnPlayerDestroyed called for unknown handle 0x%X.", handle);
	}
}

void PlayerTableSystem::FindPlayerTableBase()
{
	uintptr_t tableBase = g_pState->Domain->PlayerTable->GetPlayerTableBase();
	if (tableBase == 0)
	{
		tableBase = g_pHook->Memory->PlayerTable->GetPlayerTable();
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

void PlayerTableSystem::UpdatePlayerTable()
{
	this->UpdatePlayerData();
}

void PlayerTableSystem::Cleanup()
{
	g_pState->Domain->PlayerTable->Cleanup();
	g_pSystem->Debug->Log("[PlayerTableSystem] INFO: Cleanup completed.");
}


LivePlayer PlayerTableSystem::BuildLivePlayer(uint32_t handle, const PlayerHeader* header)
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

void PlayerTableSystem::UpdatePlayerData()
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


std::string PlayerTableSystem::WideToUtf8(const wchar_t* src, size_t maxLen)
{
	if (!src || maxLen == 0) return std::string();

	int wideLen = (int)wcsnlen(src, maxLen);
	if (wideLen == 0) return std::string();

	int utf8Len = WideCharToMultiByte(CP_UTF8, 0, src, wideLen, NULL, 0, NULL, NULL);

	std::string res(utf8Len, 0);

	WideCharToMultiByte(CP_UTF8, 0, src, wideLen, &res[0], utf8Len, NULL, NULL);

	return res;
}