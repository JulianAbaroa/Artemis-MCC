module;

#include <windows.h>

module Tables.Shadow.System;
import :PlayerTable;

import Common.Engine.Type;
import Common.Math.Type;
import Common.Team.Type;
import Layout.Player.Type;

namespace
{
    using Common::Math::Type::Vec3;
    using Common::Team::Type::Team;

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

namespace Tables::Shadow::System
{
    auto ShadowPlayerTableService::BuildLivePlayer(std::uint32_t handle, std::uintptr_t playerBase) -> AlivePlayer
    {
        AlivePlayer player{};

        player.Handle = handle;
        player.Address = playerBase;

        auto& reader = m_MemoryReaderService;

        player.ConnectionState = reader.Read<ConnectionState>(playerBase, k_ConnectionState);

        player.AliveBipedHandle = reader.Read<std::uint32_t>(playerBase, k_AliveBipedHandle);
        player.DeadBipedHandle = reader.Read<std::uint32_t>(playerBase, k_DeadBipedHandle);
        player.CurrentBipedHandle = reader.Read<std::uint32_t>(playerBase, k_CurrentBipedHandle);

        auto rawGamerTag = reader.ReadArray<wchar_t, k_GamerTagLength>(playerBase, k_GamerTag);
        player.Gamertag = this->WideToUtf8(rawGamerTag.data(), k_GamerTagLength);

        auto rawTag = reader.ReadArray<wchar_t, k_TagLength>(playerBase, k_Tag);
        player.Tag = this->WideToUtf8(rawTag.data(), k_TagLength);

        player.PrimaryWeaponHandle = reader.Read<std::uint32_t>(playerBase, k_PrimaryWeaponHandle);
        player.SecondaryWeaponHandle = reader.Read<std::uint32_t>(playerBase, k_SecondaryWeaponHandle);
        player.ObjectiveHandle = reader.Read<std::uint32_t>(playerBase, k_ObjectiveHandle);
        player.CameraPosition = reader.Read<Vec3>(playerBase, k_CameraPosition);
        player.CameraForward = reader.Read<Vec3>(playerBase, k_CameraForward);
        player.AimOffset = reader.Read<Vec3>(playerBase, k_AimOffset);
        player.LocalIndex = this->ReadLocalIndex(handle & k_HandleIndexMask);

        return player;
    }

    auto ShadowPlayerTableService::UpdatePlayerTable() -> void
    {
        this->UpdatePlayerData();
        m_PlayerStore.Publish();
    }

    auto ShadowPlayerTableService::DiscoverPlayers(std::uintptr_t tableBase) -> void
    {
        auto& reader = m_MemoryReaderService;

        for (std::uint32_t index{ 0 }; index < k_MaxPlayers; ++index)
        {
            std::uintptr_t playerBase{ tableBase + (index * k_Base) };

            std::uint32_t salt{ reader.Read<std::uint16_t>(playerBase, k_Handle) };
            if (salt == 0) continue;

            std::uint32_t handle{ (salt << k_SaltShift) | index };
            if (m_PlayerStore.HasPlayer(handle)) continue;

            if (!IsKnownConnectionState(reader.Read<ConnectionState>(playerBase, k_ConnectionState))) continue;

            m_PlayerStore.AddPlayer(handle, this->BuildLivePlayer(handle, playerBase));
        }
    }

    auto ShadowPlayerTableService::UpdatePlayerData() -> void
    {
        std::uintptr_t tableBase{ m_CaptureStore.GetPlayerTableBase() };
        if (tableBase == 0) return;

        this->DiscoverPlayers(tableBase);

        auto& reader = m_MemoryReaderService;

        std::vector<std::uint32_t> handlesToRemove{};

        m_PlayerStore.UpdatePlayers([&](std::uint32_t handle, AlivePlayer& player) {
            std::uint32_t index{ handle & k_HandleIndexMask };
            std::uintptr_t playerBase{ tableBase + (index * k_Base) };

            std::uint32_t rawHandleInMemory{ reader.Read<std::uint32_t>(playerBase, k_Handle) };

            std::uint32_t handleInMemory{ (rawHandleInMemory << k_SaltShift) | index };
            if (handleInMemory != handle)
            {
                handlesToRemove.push_back(handle);
                return;
            }

            player.ConnectionState = reader.Read<ConnectionState>(playerBase, k_ConnectionState);

            player.Team = reader.Read<Team>(playerBase, k_Team);

            player.AliveBipedHandle = reader.Read<std::uint32_t>(playerBase, k_AliveBipedHandle);
            player.DeadBipedHandle = reader.Read<std::uint32_t>(playerBase, k_DeadBipedHandle);
            player.CurrentBipedHandle = reader.Read<std::uint32_t>(playerBase, k_CurrentBipedHandle);

            player.PrimaryWeaponHandle = reader.Read<std::uint32_t>(playerBase, k_PrimaryWeaponHandle);
            player.SecondaryWeaponHandle = reader.Read<std::uint32_t>(playerBase, k_SecondaryWeaponHandle);
            player.ObjectiveHandle = reader.Read<std::uint32_t>(playerBase, k_ObjectiveHandle);
            player.CameraPosition = reader.Read<Vec3>(playerBase, k_CameraPosition);
            player.CameraForward = reader.Read<Vec3>(playerBase, k_CameraForward);
            player.AimOffset = reader.Read<Vec3>(playerBase, k_AimOffset);
            player.LocalIndex = this->ReadLocalIndex(index);
        });

        for (std::uint32_t handle : handlesToRemove)
        {
            m_PlayerStore.RemovePlayer(handle);
        }
    }

    auto ShadowPlayerTableService::ReadLocalIndex(std::uint32_t index) -> std::optional<std::uint8_t>
    {
        const std::uintptr_t mappingBase{ m_CaptureStore.GetMappingBase() };
        if (mappingBase == 0) return std::nullopt;

        const std::int32_t inputUser{ m_MemoryReaderService.Read<std::int32_t>(mappingBase,
            k_PlayerInputUser + (index * k_EntryStride)) };

        if (inputUser < 0 || static_cast<std::size_t>(inputUser) >= k_MaxLocalUsers) return std::nullopt;

        return static_cast<std::uint8_t>(inputUser);
    }

    auto ShadowPlayerTableService::Cleanup() -> void
    {
        m_PlayerStore.Cleanup();

        m_Log.Message("[ShadowPlayerTableService] INFO: Cleanup completed.");
    }

    auto ShadowPlayerTableService::WideToUtf8(const wchar_t* source, std::size_t maxLength) -> std::string
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