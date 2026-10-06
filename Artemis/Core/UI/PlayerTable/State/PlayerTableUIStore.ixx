export module UI.PlayerTable.State;

import Export.Tick.Type;
import UI.PlayerTable.Type;
import std;

export namespace UI::PlayerTable::State
{
    // Snapshot of the player table with its alive players sorted by handle.
    class PlayerTableUIStore
    {
    private:
        using PlayerTable = Export::Tick::Type::PlayerTable;
        using AlivePlayer = Export::Tick::Type::AlivePlayer;

        using Players = UI::PlayerTable::Type::Players;

    public:
        PlayerTableUIStore() = default;
        ~PlayerTableUIStore() = default;

        PlayerTableUIStore(const PlayerTableUIStore&) = delete;
        auto operator=(const PlayerTableUIStore&) -> PlayerTableUIStore& = delete;

        // Checks whether the table is the stored snapshot.
        auto IsSameSnapshot(const std::shared_ptr<const PlayerTable>& table) const -> bool;

        // Replaces the snapshot and its players.
        // note: The players point into the table, so both must be replaced together.
        auto SetSnapshot(std::shared_ptr<const PlayerTable> table, Players players) -> void;

        // Returns the alive players sorted by handle.
        auto GetPlayers() const -> const Players&;

        // Returns the number of players of the snapshot.
        auto GetPlayerCount() const -> std::size_t;

        // Drops the snapshot and its players.
        auto Cleanup() -> void;

    private:
        std::shared_ptr<const PlayerTable> m_Snapshot{};
        Players m_Players{};
    };
}