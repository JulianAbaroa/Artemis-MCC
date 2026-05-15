#include "pch.h"

// Header.
#include "System_PlayerTable.h"

// Types.
#include "Core/Types/Domain/Player/PlayerOffsets.h"
#include "Core/Types/Domain/Player/PlayerSizes.h"

// --- States ---

#include "Core/States/Domain/Player/State_PlayerTable.h"

// --- Systems ---

#include "Core/Systems/Infrastructure/Engine/Memory/System_MemoryReader.h"

#include "Core/Systems/Interface/Debug/System_Debug.h"

#include <vector>

// Called from 'Hook_CreatePlayer::HookedCreatePlayer', its responsible
// of adding every new created player to the Artemis player table.
void System_PlayerTable::OnPlayerCreated(uint32_t handle)
{
	// Gets the base memory address of the player table.
	uintptr_t tableBase = m_Deps.State_PlayerTable.GetPlayerTableBase();
	if (tableBase == 0) return;

	// We get the index of this player. (e.g. 0x00, 0x01, 0x02, ...)
	uint32_t index = handle & 0xFFFF;

	// If this index was owned by a previous player, and now its being used
	// by this new player, it means the previous player left, so we have to remove it.
	m_Deps.State_PlayerTable.RemovePlayerIf(
		[index](uint32_t oldHandle, const LivePlayer& p) {
			return (oldHandle & 0xFFFF) == index;
		});

	// Memory address arithmetic to find the base memory address of this player struct.
	uintptr_t playerBase = tableBase + (index * PlayerSizes::Base);

	// We build and add this new player to the Artemis player table.
	m_Deps.State_PlayerTable.AddPlayer(
		handle, this->BuildLivePlayer(handle, playerBase));
}

// Called from 'Thread_AI::Run', its responsible of updating the player table.
void System_PlayerTable::UpdatePlayerTable()
{
	this->UpdatePlayerData();
}

void System_PlayerTable::Cleanup()
{
	m_Deps.State_PlayerTable.Cleanup();
	m_Deps.System_Debug.Log("[PlayerTableSystem] INFO: Cleanup completed.");
}

// Responsible of reading and building the base data for a new created player.
LivePlayer System_PlayerTable::BuildLivePlayer(uint32_t handle, uintptr_t playerBase)
{
	namespace Player = PlayerOffsets;

	LivePlayer player{};

	player.Handle = handle;
	player.Address = playerBase;

	auto& reader = m_Deps.System_MemoryReader;

	// Connection State.
	player.ConnectionState = reader.Read<ConnectionState>(playerBase, Player::ConnectionState);

	// Bipeds.
	player.AliveBipedHandle = reader.Read<uint32_t>(playerBase, Player::AliveBipedHandle);
	player.DeadBipedHandle = reader.Read<uint32_t>(playerBase, Player::DeadBipedHandle);
	player.CurrentBipedHandle = reader.Read<uint32_t>(playerBase, Player::CurrentBipedHandle);

	// Gamertag & Tag.
	auto rawGamerTag = reader.ReadArray<wchar_t, 16>(playerBase, Player::GamerTag);
	player.Gamertag = this->WideToUtf8(rawGamerTag.data(), 16);

	auto rawTag = reader.ReadArray<wchar_t, 4>(playerBase, Player::Tag);
	player.Tag = this->WideToUtf8(rawTag.data(), 4);

	// Weapons.
	player.PrimaryWeaponHandle = reader.Read<uint32_t>(playerBase, Player::PrimaryWeaponHandle);
	player.SecondaryWeaponHandle = reader.Read<uint32_t>(playerBase, Player::SecondaryWeaponHandle);
	player.ObjectiveHandle = reader.Read<uint32_t>(playerBase, Player::ObjectiveHandle);
	player.WeaponPosition = reader.ReadArray<float, 3>(playerBase, Player::WeaponPosition);
	player.WeaponForward = reader.ReadArray<float, 3>(playerBase, Player::WeaponForward);
	player.AimOffset = reader.ReadArray<float, 3>(playerBase, Player::AimOffset);

	return player;
}

// Responsible of updating every player data on Artemis player table.
void System_PlayerTable::UpdatePlayerData()
{
	// Gets the base memory address of the player table.
	uintptr_t tableBase = m_Deps.State_PlayerTable.GetPlayerTableBase();
	if (tableBase == 0) return;

	namespace Player = PlayerOffsets;

	auto& reader = m_Deps.System_MemoryReader;

	std::vector<uint32_t> handlesToRemove;

	// We iterate through Artemis player table, to read and update its values.
	m_Deps.State_PlayerTable.UpdatePlayers(
		[tableBase, &handlesToRemove, &reader](uint32_t handle, LivePlayer& player) {
			uint32_t index = handle & 0xFFFF;
			uintptr_t playerBase = tableBase + (index * PlayerSizes::Base);

			// We get the handle of this player in memory. 
			// (e.g. 73 EC 00 00, 74 EC 00 00, ...)
			uint32_t rawHandleInMemory = reader.Read<uint32_t>(playerBase, Player::Handle);

			// Since the raw handle doens't contains the index, we add it.
			uint32_t handleInMemory = (rawHandleInMemory << 16) | index;

			// If these two handles are different, it means the player that owns this
			// handle on Artemis player table has left.
			// (e.g. 79 EC 00 04 != 73 EC 00 04)
			if (handleInMemory != handle)
			{
				handlesToRemove.push_back(handle);
				return;
			}

			// Connection State.
			player.ConnectionState = reader.Read<ConnectionState>(playerBase, Player::ConnectionState);

			// Bipeds.
			player.AliveBipedHandle = reader.Read<uint32_t>(playerBase, Player::AliveBipedHandle);
			player.DeadBipedHandle = reader.Read<uint32_t>(playerBase, Player::DeadBipedHandle);
			player.CurrentBipedHandle = reader.Read<uint32_t>(playerBase, Player::CurrentBipedHandle);

			// Weapons.
			player.PrimaryWeaponHandle = reader.Read<uint32_t>(playerBase, Player::PrimaryWeaponHandle);
			player.SecondaryWeaponHandle = reader.Read<uint32_t>(playerBase, Player::SecondaryWeaponHandle);
			player.ObjectiveHandle = reader.Read<uint32_t>(playerBase, Player::ObjectiveHandle);
			player.WeaponPosition = reader.ReadArray<float, 3>(playerBase, Player::WeaponPosition);
			player.WeaponForward = reader.ReadArray<float, 3>(playerBase, Player::WeaponForward);
			player.AimOffset = reader.ReadArray<float, 3>(playerBase, Player::AimOffset);

			player.IsAlive = player.AliveBipedHandle == 0xFFFFFFFF ? false : true;
		}
	);

	// We remove the players that had left.
	for (uint32_t handle : handlesToRemove)
	{
		m_Deps.State_PlayerTable.RemovePlayer(handle);
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