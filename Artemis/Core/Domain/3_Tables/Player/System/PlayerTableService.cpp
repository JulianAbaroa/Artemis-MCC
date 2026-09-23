module;

#include <windows.h>

module Tables.Player.System;

import Common.Math.Type;
import Common.Team.Type;

namespace
{
	namespace Sizes = Tables::Player::Type::Size;
	namespace Offset = Tables::Player::Type::Offset;

	using Vec3 = Common::Math::Type::Vec3;
	using Team = Common::Team::Type::Team;
	using ConnectionState = Tables::Player::Type::Alive::ConnectionState;
}

namespace Tables::Player::System
{
	auto PlayerTableService::OnPlayerCreated(std::uint32_t handle) -> void
	{
		std::uintptr_t tableBase = m_PlayerStore.GetBase();
		if (tableBase == 0) return;

		std::uint32_t index = handle & 0xFFFF;

		m_PlayerStore.RemovePlayerIf(
			[index](std::uint32_t oldHandle, const auto& player) {
				return (oldHandle & 0xFFFF) == index;
			});

		std::uintptr_t playerBase = tableBase + (index * Sizes::Base);

		m_PlayerStore.AddPlayer(
			handle, this->BuildLivePlayer(handle, playerBase));
	}

	auto PlayerTableService::BuildLivePlayer(std::uint32_t handle, std::uintptr_t playerBase) -> AlivePlayer
	{
		AlivePlayer player{};

		player.Handle = handle;
		player.Address = playerBase;

		auto& reader = m_MemoryReaderService;

		player.ConnectionState = reader.Read<ConnectionState>(playerBase, Offset::ConnectionState);

		player.AliveBipedHandle = reader.Read<std::uint32_t>(playerBase, Offset::AliveBipedHandle);
		player.DeadBipedHandle = reader.Read<std::uint32_t>(playerBase, Offset::DeadBipedHandle);
		player.CurrentBipedHandle = reader.Read<std::uint32_t>(playerBase, Offset::CurrentBipedHandle);

		auto rawGamerTag = reader.ReadArray<wchar_t, 16>(playerBase, Offset::GamerTag);
		player.Gamertag = this->WideToUtf8(rawGamerTag.data(), 16);

		auto rawTag = reader.ReadArray<wchar_t, 4>(playerBase, Offset::Tag);
		player.Tag = this->WideToUtf8(rawTag.data(), 4);

		player.PrimaryWeaponHandle = reader.Read<std::uint32_t>(playerBase, Offset::PrimaryWeaponHandle);
		player.SecondaryWeaponHandle = reader.Read<std::uint32_t>(playerBase, Offset::SecondaryWeaponHandle);
		player.ObjectiveHandle = reader.Read<std::uint32_t>(playerBase, Offset::ObjectiveHandle);
		player.WeaponPosition = reader.Read<Vec3>(playerBase, Offset::WeaponPosition);
		player.WeaponForward = reader.Read<Vec3>(playerBase, Offset::WeaponForward);
		player.AimOffset = reader.Read<Vec3>(playerBase, Offset::AimOffset);

		return player;
	}

	void PlayerTableService::UpdatePlayerTable()
	{
		this->UpdatePlayerData();
		m_PlayerStore.Publish();
	}

	void PlayerTableService::UpdatePlayerData()
	{
		std::uintptr_t tableBase = m_PlayerStore.GetBase();
		if (tableBase == 0) return;

		auto& reader = m_MemoryReaderService;

		std::vector<std::uint32_t> handlesToRemove;

		m_PlayerStore.UpdatePlayers(
			[&](std::uint32_t handle, AlivePlayer& player) {
			std::uint32_t index = handle & 0xFFFF;
			std::uintptr_t playerBase = tableBase + (index * Sizes::Base);
	
			std::uint32_t rawHandleInMemory = reader.Read<std::uint32_t>(playerBase, Offset::Handle);
	
			std::uint32_t handleInMemory = (rawHandleInMemory << 16) | index;
			if (handleInMemory != handle)
			{
				handlesToRemove.push_back(handle);
				return;
			}
	
			player.ConnectionState = reader.Read<ConnectionState>(playerBase, Offset::ConnectionState);
	
			player.Team = reader.Read<Team>(playerBase, Offset::Team);
	
			player.AliveBipedHandle = reader.Read<std::uint32_t>(playerBase, Offset::AliveBipedHandle);
			player.DeadBipedHandle = reader.Read<std::uint32_t>(playerBase, Offset::DeadBipedHandle);
			player.CurrentBipedHandle = reader.Read<std::uint32_t>(playerBase, Offset::CurrentBipedHandle);
	
			player.PrimaryWeaponHandle = reader.Read<std::uint32_t>(playerBase, Offset::PrimaryWeaponHandle);
			player.SecondaryWeaponHandle = reader.Read<std::uint32_t>(playerBase, Offset::SecondaryWeaponHandle);
			player.ObjectiveHandle = reader.Read<std::uint32_t>(playerBase, Offset::ObjectiveHandle);
			player.WeaponPosition = reader.Read<Vec3>(playerBase, Offset::WeaponPosition);
			player.WeaponForward = reader.Read<Vec3>(playerBase, Offset::WeaponForward);
			player.AimOffset = reader.Read<Vec3>(playerBase, Offset::AimOffset);
		});

		for (std::uint32_t handle : handlesToRemove)
		{
			m_PlayerStore.RemovePlayer(handle);
		}
	}

	void PlayerTableService::Cleanup()
	{
		m_PlayerStore.Cleanup();

		m_LogsService.Message("[PlayerTableService] INFO: Cleanup completed.");
	}

	// --- Helpers ---

	std::string PlayerTableService::WideToUtf8(const wchar_t* source, std::size_t maxLength)
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
}