#include "pch.h"

#include "System_PlayerTable.h"

#include "Core/Types/Sources/Tables/Player/PlayerOffsets.h"
#include "Core/Types/Sources/Tables/Player/PlayerSizes.h"

#include "Core/States/Sources/Tables/Player/State_PlayerTable.h"

#include "Core/Systems/Other/Memory/Reader/System_MemoryReader.h"
#include "Core/Systems/Other/Logs/System_Logs.h"

#include <vector>

void System_PlayerTable::OnPlayerCreated(uint32_t handle)
{
	uintptr_t tableBase = m_Deps.State_PlayerTable.GetBase();
	if (tableBase == 0) return;

	uint32_t index = handle & 0xFFFF;

	m_Deps.State_PlayerTable.RemovePlayerIf(
		[index](uint32_t oldHandle, const LivePlayer& player) {
			return (oldHandle & 0xFFFF) == index;
		});

	uintptr_t playerBase = tableBase + (index * PlayerSizes::Base);

	m_Deps.State_PlayerTable.AddPlayer(
		handle, this->BuildLivePlayer(handle, playerBase));
}

LivePlayer System_PlayerTable::BuildLivePlayer(
	uint32_t handle, uintptr_t playerBase)
{
	namespace Player = PlayerOffsets;

	LivePlayer player{};

	player.Handle = handle;
	player.Address = playerBase;

	auto& reader = m_Deps.System_MemoryReader;

	player.ConnectionState = reader.Read<ConnectionState>(playerBase, Player::ConnectionState);

	player.AliveBipedHandle = reader.Read<uint32_t>(playerBase, Player::AliveBipedHandle);
	player.DeadBipedHandle = reader.Read<uint32_t>(playerBase, Player::DeadBipedHandle);
	player.CurrentBipedHandle = reader.Read<uint32_t>(playerBase, Player::CurrentBipedHandle);

	auto rawGamerTag = reader.ReadArray<wchar_t, 16>(playerBase, Player::GamerTag);
	player.Gamertag = this->WideToUtf8(rawGamerTag.data(), 16);

	auto rawTag = reader.ReadArray<wchar_t, 4>(playerBase, Player::Tag);
	player.Tag = this->WideToUtf8(rawTag.data(), 4);

	player.PrimaryWeaponHandle = reader.Read<uint32_t>(playerBase, Player::PrimaryWeaponHandle);
	player.SecondaryWeaponHandle = reader.Read<uint32_t>(playerBase, Player::SecondaryWeaponHandle);
	player.ObjectiveHandle = reader.Read<uint32_t>(playerBase, Player::ObjectiveHandle);
	player.WeaponPosition = reader.ReadArray<float, 3>(playerBase, Player::WeaponPosition);
	player.WeaponForward = reader.ReadArray<float, 3>(playerBase, Player::WeaponForward);
	player.AimOffset = reader.ReadArray<float, 3>(playerBase, Player::AimOffset);

	return player;
}

void System_PlayerTable::UpdatePlayerTable()
{
	this->UpdatePlayerData();
	m_Deps.State_PlayerTable.Publish();
}

void System_PlayerTable::UpdatePlayerData()
{
	uintptr_t tableBase = m_Deps.State_PlayerTable.GetBase();
	if (tableBase == 0) return;

	namespace Player = PlayerOffsets;

	auto& reader = m_Deps.System_MemoryReader;

	std::vector<uint32_t> handlesToRemove;

	m_Deps.State_PlayerTable.UpdatePlayers(
		[&](uint32_t handle, LivePlayer& player) {
		uint32_t index = handle & 0xFFFF;
		uintptr_t playerBase = tableBase + (index * PlayerSizes::Base);
	
		uint32_t rawHandleInMemory = reader.Read<uint32_t>(playerBase, Player::Handle);
	
		uint32_t handleInMemory = (rawHandleInMemory << 16) | index;
		if (handleInMemory != handle)
		{
			handlesToRemove.push_back(handle);
			return;
		}
	
		player.ConnectionState = reader.Read<ConnectionState>(playerBase, Player::ConnectionState);
	
		player.Team = reader.Read<Team>(playerBase, Player::Team);
	
		player.AliveBipedHandle = reader.Read<uint32_t>(playerBase, Player::AliveBipedHandle);
		player.DeadBipedHandle = reader.Read<uint32_t>(playerBase, Player::DeadBipedHandle);
		player.CurrentBipedHandle = reader.Read<uint32_t>(playerBase, Player::CurrentBipedHandle);
	
		player.PrimaryWeaponHandle = reader.Read<uint32_t>(playerBase, Player::PrimaryWeaponHandle);
		player.SecondaryWeaponHandle = reader.Read<uint32_t>(playerBase, Player::SecondaryWeaponHandle);
		player.ObjectiveHandle = reader.Read<uint32_t>(playerBase, Player::ObjectiveHandle);
		player.WeaponPosition = reader.ReadArray<float, 3>(playerBase, Player::WeaponPosition);
		player.WeaponForward = reader.ReadArray<float, 3>(playerBase, Player::WeaponForward);
		player.AimOffset = reader.ReadArray<float, 3>(playerBase, Player::AimOffset);
	});

	for (uint32_t handle : handlesToRemove)
	{
		m_Deps.State_PlayerTable.RemovePlayer(handle);
	}
}

void System_PlayerTable::Cleanup()
{
	m_Deps.State_PlayerTable.Cleanup();

	m_Deps.System_Logs.Log("[PlayerTable] INFO:"
		" Cleanup completed.");
}

// --- Helpers ---

std::string System_PlayerTable::WideToUtf8(
	const wchar_t* source, size_t maxLength)
{
	if (!source || maxLength == 0) return std::string();

	int wideLength = (int)wcsnlen(source, maxLength);
	if (wideLength == 0) return std::string();

	int utf8Length = WideCharToMultiByte(CP_UTF8, 0, source, 
		wideLength, NULL, 0, NULL, NULL);

	std::string result(utf8Length, 0);

	WideCharToMultiByte(CP_UTF8, 0, source, wideLength, 
		&result[0], utf8Length, NULL, NULL);

	return result;
}