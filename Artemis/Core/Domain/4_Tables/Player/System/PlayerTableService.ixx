export module Tables.Player.System;

import Service.Logs.System;
import Platform.Memory.System;
import Tables.Player.Type;
import Tables.Player.State;
import std;

export namespace Tables::Player::System
{
    // Reads the players of the engine player table into the store.
    class PlayerTableService
    {
    private:
        using LogsService = Service::Logs::System::LogsService;

        using MemoryReaderService = Platform::Memory::System::MemoryReaderService;

        using AlivePlayer = Tables::Player::Type::Alive::AlivePlayer;

        using PlayerTableStore = Tables::Player::State::PlayerTableStore;

    public:
        PlayerTableService(LogsService& logsService,
            MemoryReaderService& memoryReaderService, PlayerTableStore& playerStore) :
            m_LogsService(logsService), m_MemoryReaderService(memoryReaderService),
            m_PlayerStore(playerStore) {}
        ~PlayerTableService() = default;

        // Adds the players found in the table slots, refreshes the stored ones and publishes the table.
        // note: Removes the players whose handle no longer matches the one in memory.
        auto UpdatePlayerTable() -> void;

        // Clears the stored table base and players.
        auto Cleanup() -> void;

    private:
        LogsService& m_LogsService;
        MemoryReaderService& m_MemoryReaderService;
        PlayerTableStore& m_PlayerStore;

        auto BuildLivePlayer(std::uint32_t handle, std::uintptr_t playerBase) -> AlivePlayer;

        // Adds the occupied slots of the table that are not stored yet.
        // note: A slot is occupied when its salt is not zero and its connection state is a known one.
        auto DiscoverPlayers(std::uintptr_t tableBase) -> void;

        auto UpdatePlayerData() -> void;

        auto WideToUtf8(const wchar_t* source, std::size_t maxLength) -> std::string;
    };
}