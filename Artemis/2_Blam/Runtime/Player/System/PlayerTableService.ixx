export module Blam.Runtime.Player.System;

import Common.Log.System;
import Blam.Layout.Snapshot.Type;
import Blam.Runtime.Player.Type;
import Blam.Runtime.Player.State;
import std;

export namespace Blam::Runtime::Player::System
{
    // Reads the players of the engine player table into the store.
    class PlayerTableService
    {
    private:
        using Log = Common::Log::System::Log;

        using AlivePlayer = Blam::Runtime::Player::Type::Alive::AlivePlayer;

        using PlayerTableStore = Blam::Runtime::Player::State::PlayerTableStore;

        using Snapshot = Blam::Layout::Snapshot::Type::Snapshot;

    public:
        PlayerTableService(Log& log, PlayerTableStore& playerStore) :
            m_Log(log), m_PlayerStore(playerStore) {}
        ~PlayerTableService() = default;

        // Adds the players found in the table slots of the snapshot, refreshes the stored ones and publishes the table.
        // note: Removes the players whose handle no longer matches the one in the snapshot. Publishes the table as it was while the player array is not captured.
        auto UpdatePlayerTable(const Snapshot& snapshot) -> void;

        // Clears the stored players.
        auto Cleanup() -> void;

    private:
        Log& m_Log;
        PlayerTableStore& m_PlayerStore;

        auto BuildLivePlayer(const Snapshot& snapshot, std::uint32_t handle, std::size_t entry) -> AlivePlayer;

        // Adds the occupied slots of the table that are not stored yet.
        // note: A slot is occupied when its salt is not zero and its connection state is a known one.
        auto DiscoverPlayers(const Snapshot& snapshot) -> void;

        auto UpdatePlayerData(const Snapshot& snapshot) -> void;

        // return: Local player of the table index, or empty when it is not local or the mapping is not captured.
        auto ReadLocalIndex(const Snapshot& snapshot, std::uint32_t index) -> std::optional<std::uint8_t>;
    };
}