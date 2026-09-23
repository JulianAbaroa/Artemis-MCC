export module UI.PlayerTable.State;

import Export.Tick.Type;
import std;

export namespace UI::PlayerTable::State
{
	class PlayerTableUIStore
	{
	private:
		using PlayerTable = Export::Tick::Type::PlayerTable;
		using AlivePlayer = Export::Tick::Type::AlivePlayer;

	public:
		using Players = std::vector<const AlivePlayer*>;

		PlayerTableUIStore() = default;
		~PlayerTableUIStore() = default;

		PlayerTableUIStore(const PlayerTableUIStore&) = delete;
		PlayerTableUIStore& operator=(const PlayerTableUIStore&) = delete;

		auto IsSameSnapshot(const std::shared_ptr<const PlayerTable>& table) const -> bool;
		auto SetSnapshot(std::shared_ptr<const PlayerTable> table, Players players) -> void;

		auto GetPlayers() const -> const Players&;
		auto GetPlayerCount() const -> std::size_t;

		auto Cleanup() -> void;

	private:
		std::shared_ptr<const PlayerTable> m_Snapshot{};
		Players m_Players{};
	};
}