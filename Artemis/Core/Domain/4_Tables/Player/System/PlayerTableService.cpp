module;

#include <windows.h>

module Tables.Player.System;

import Common.Engine.Type;
import Common.Math.Type;
import Common.Team.Type;
import Layout.Player.Type;
import Layout.Snapshot.Type;

namespace
{
    using Common::Math::Type::Vec3;
    using Common::Team::Type::Team;

    using Layout::Snapshot::Type::RegionId;

    using Tables::Player::Type::Alive::ConnectionState;

    using Layout::Player::Type::Size::k_Base;
    using Layout::Player::Type::Size::k_TagLength;

    using Layout::Player::Type::Offset::k_Handle;
    using Layout::Player::Type::Offset::k_ConnectionState;
    using Layout::Player::Type::Offset::k_AliveBipedHandle;
    using Layout::Player::Type::Offset::k_DeadBipedHandle;
    using Layout::Player::Type::Offset::k_CurrentBipedHandle;
    using Layout::Player::Type::Offset::k_CameraPosition;
    using Layout::Player::Type::Offset::k_CameraForward;
    using Layout::Player::Type::Offset::k_AimOffset;
    using Layout::Player::Type::Offset::k_PrimaryWeaponHandle;
    using Layout::Player::Type::Offset::k_SecondaryWeaponHandle;
    using Layout::Player::Type::Offset::k_ObjectiveHandle;
    using Layout::Player::Type::Offset::k_Team;
    using Layout::Player::Type::Offset::k_GamerTag;
    using Layout::Player::Type::Offset::k_Tag;

    using Layout::Player::Type::Mapping::k_PlayerInputUser;
    using Layout::Player::Type::Mapping::k_EntryStride;

    using Common::Engine::Type::k_MaxLocalUsers;
    using Common::Engine::Type::k_HandleIndexMask;
    using Common::Engine::Type::k_SaltShift;
    using Common::Engine::Type::k_MaxPlayers;
    using Common::Engine::Type::k_GamerTagLength;

    // Whether the value is a connection state the engine uses.
    auto IsKnownConnectionState(ConnectionState state) -> bool
    {
        return state == ConnectionState::Connected ||
            state == ConnectionState::Disconnected ||
            state == ConnectionState::Connecting;
    }
}

namespace Tables::Player::System
{
    auto PlayerTableService::BuildLivePlayer(const Snapshot& snapshot, std::uint32_t handle, std::size_t entry) -> AlivePlayer
    {
        AlivePlayer player{};

        player.Handle = handle;
        player.Address = snapshot.Regions[static_cast<std::size_t>(RegionId::PlayerArray)].OriginalAddress + entry;

        player.ConnectionState = snapshot.Read<ConnectionState>(RegionId::PlayerArray, entry + k_ConnectionState);

        player.AliveBipedHandle = snapshot.Read<std::uint32_t>(RegionId::PlayerArray, entry + k_AliveBipedHandle);
        player.DeadBipedHandle = snapshot.Read<std::uint32_t>(RegionId::PlayerArray, entry + k_DeadBipedHandle);
        player.CurrentBipedHandle = snapshot.Read<std::uint32_t>(RegionId::PlayerArray, entry + k_CurrentBipedHandle);

        auto rawGamerTag = snapshot.ReadArray<wchar_t, k_GamerTagLength>(RegionId::PlayerArray, entry + k_GamerTag);
        player.Gamertag = this->WideToUtf8(rawGamerTag.data(), k_GamerTagLength);

        auto rawTag = snapshot.ReadArray<wchar_t, k_TagLength>(RegionId::PlayerArray, entry + k_Tag);
        player.Tag = this->WideToUtf8(rawTag.data(), k_TagLength);

        player.PrimaryWeaponHandle = snapshot.Read<std::uint32_t>(RegionId::PlayerArray, entry + k_PrimaryWeaponHandle);
        player.SecondaryWeaponHandle = snapshot.Read<std::uint32_t>(RegionId::PlayerArray, entry + k_SecondaryWeaponHandle);
        player.ObjectiveHandle = snapshot.Read<std::uint32_t>(RegionId::PlayerArray, entry + k_ObjectiveHandle);
        player.CameraPosition = snapshot.Read<Vec3>(RegionId::PlayerArray, entry + k_CameraPosition);
        player.CameraForward = snapshot.Read<Vec3>(RegionId::PlayerArray, entry + k_CameraForward);
        player.AimOffset = snapshot.Read<Vec3>(RegionId::PlayerArray, entry + k_AimOffset);
        player.LocalIndex = this->ReadLocalIndex(snapshot, handle & k_HandleIndexMask);

        return player;
    }

    auto PlayerTableService::UpdatePlayerTable(const Snapshot& snapshot) -> void
    {
        this->UpdatePlayerData(snapshot);
        m_PlayerStore.Publish();
    }

    auto PlayerTableService::DiscoverPlayers(const Snapshot& snapshot) -> void
    {
        for (std::uint32_t index{ 0 }; index < k_MaxPlayers; ++index)
        {
            std::size_t entry{ index * k_Base };

            std::uint32_t salt{ snapshot.Read<std::uint16_t>(RegionId::PlayerArray, entry + k_Handle) };
            if (salt == 0) continue;

            std::uint32_t handle{ (salt << k_SaltShift) | index };
            if (m_PlayerStore.HasPlayer(handle)) continue;

            if (!IsKnownConnectionState(snapshot.Read<ConnectionState>(RegionId::PlayerArray, entry + k_ConnectionState))) continue;

            m_PlayerStore.AddPlayer(handle, this->BuildLivePlayer(snapshot, handle, entry));
        }
    }

    auto PlayerTableService::UpdatePlayerData(const Snapshot& snapshot) -> void
    {
        if (!snapshot.IsCaptured(RegionId::PlayerArray)) return;

        this->DiscoverPlayers(snapshot);

        std::vector<std::uint32_t> handlesToRemove{};

        m_PlayerStore.UpdatePlayers([&](std::uint32_t handle, AlivePlayer& player) {
            std::uint32_t index{ handle & k_HandleIndexMask };
            std::size_t entry{ index * k_Base };

            std::uint32_t rawHandleInMemory{ snapshot.Read<std::uint32_t>(RegionId::PlayerArray, entry + k_Handle) };

            std::uint32_t handleInMemory{ (rawHandleInMemory << k_SaltShift) | index };
            if (handleInMemory != handle)
            {
                handlesToRemove.push_back(handle);
                return;
            }

            player.ConnectionState = snapshot.Read<ConnectionState>(RegionId::PlayerArray, entry + k_ConnectionState);

            player.Team = snapshot.Read<Team>(RegionId::PlayerArray, entry + k_Team);

            player.AliveBipedHandle = snapshot.Read<std::uint32_t>(RegionId::PlayerArray, entry + k_AliveBipedHandle);
            player.DeadBipedHandle = snapshot.Read<std::uint32_t>(RegionId::PlayerArray, entry + k_DeadBipedHandle);
            player.CurrentBipedHandle = snapshot.Read<std::uint32_t>(RegionId::PlayerArray, entry + k_CurrentBipedHandle);

            player.PrimaryWeaponHandle = snapshot.Read<std::uint32_t>(RegionId::PlayerArray, entry + k_PrimaryWeaponHandle);
            player.SecondaryWeaponHandle = snapshot.Read<std::uint32_t>(RegionId::PlayerArray, entry + k_SecondaryWeaponHandle);
            player.ObjectiveHandle = snapshot.Read<std::uint32_t>(RegionId::PlayerArray, entry + k_ObjectiveHandle);
            player.CameraPosition = snapshot.Read<Vec3>(RegionId::PlayerArray, entry + k_CameraPosition);
            player.CameraForward = snapshot.Read<Vec3>(RegionId::PlayerArray, entry + k_CameraForward);
            player.AimOffset = snapshot.Read<Vec3>(RegionId::PlayerArray, entry + k_AimOffset);
            player.LocalIndex = this->ReadLocalIndex(snapshot, index);
        });

        for (std::uint32_t handle : handlesToRemove)
        {
            m_PlayerStore.RemovePlayer(handle);
        }
    }

    auto PlayerTableService::ReadLocalIndex(const Snapshot& snapshot, std::uint32_t index) -> std::optional<std::uint8_t>
    {
        if (!snapshot.IsCaptured(RegionId::PlayerMapping)) return std::nullopt;

        const std::int32_t inputUser{ snapshot.Read<std::int32_t>(RegionId::PlayerMapping,
            k_PlayerInputUser + (index * k_EntryStride)) };

        if (inputUser < 0 || static_cast<std::size_t>(inputUser) >= k_MaxLocalUsers) return std::nullopt;

        return static_cast<std::uint8_t>(inputUser);
    }

    auto PlayerTableService::Cleanup() -> void
    {
        m_PlayerStore.Cleanup();

        m_Log.Message("[PlayerTableService] INFO: Cleanup completed.");
    }

    auto PlayerTableService::WideToUtf8(const wchar_t* source, std::size_t maxLength) -> std::string
    {
        if (!source || maxLength == 0) return std::string{};

        int wideLength{ (int)wcsnlen(source, maxLength) };
        if (wideLength == 0) return std::string{};

        int utf8Length{ WideCharToMultiByte(CP_UTF8, 0, source,
            wideLength, NULL, 0, NULL, NULL) };

        std::string result(utf8Length, 0);

        WideCharToMultiByte(CP_UTF8, 0, source, wideLength,
            &result[0], utf8Length, NULL, NULL);

        return result;
    }
}