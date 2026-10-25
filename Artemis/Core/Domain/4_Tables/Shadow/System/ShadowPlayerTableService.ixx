export module Tables.Shadow.System:PlayerTable;

import Common.Log.System;
import Platform.Memory.System;
import Platform.Capture.State;
import Tables.Player.Type;
import Tables.Player.State;
import std;

export namespace Tables::Shadow::System
{
    // Old live read of the players, kept to compare it with the one that reads the snapshot (M4a, D188).
    // note: Temporary copy of the PlayerTableService of before the snapshot. M4b deletes it.
    class ShadowPlayerTableService
    {
    private:
        using Log = Common::Log::System::Log;

        using MemoryReaderService = Platform::Memory::System::MemoryReaderService;

        using CaptureStore = Platform::Capture::State::CaptureStore;

        using AlivePlayer = Tables::Player::Type::Alive::AlivePlayer;

        using PlayerTableStore = Tables::Player::State::PlayerTableStore;

    public:
        ShadowPlayerTableService(Log& log,
            MemoryReaderService& memoryReaderService, CaptureStore& captureStore, PlayerTableStore& playerStore) :
            m_Log(log), m_MemoryReaderService(memoryReaderService), m_CaptureStore(captureStore),
            m_PlayerStore(playerStore) {}
        ~ShadowPlayerTableService() = default;

        // Adds the players found in the table slots, refreshes the stored ones and publishes the table.
        // note: Removes the players whose handle no longer matches the one in memory.
        auto UpdatePlayerTable() -> void;

        // Clears the stored table base and players.
        auto Cleanup() -> void;

    private:
        Log& m_Log;
        MemoryReaderService& m_MemoryReaderService;
        CaptureStore& m_CaptureStore;
        PlayerTableStore& m_PlayerStore;

        auto BuildLivePlayer(std::uint32_t handle, std::uintptr_t playerBase) -> AlivePlayer;

        // Adds the occupied slots of the table that are not stored yet.
        // note: A slot is occupied when its salt is not zero and its connection state is a known one.
        auto DiscoverPlayers(std::uintptr_t tableBase) -> void;

        auto UpdatePlayerData() -> void;

        // return: Local player of the table index, or empty when it is not local or the mapping is not found.
        auto ReadLocalIndex(std::uint32_t index) -> std::optional<std::uint8_t>;

        auto WideToUtf8(const wchar_t* source, std::size_t maxLength) -> std::string;
    };
}