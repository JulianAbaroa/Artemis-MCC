export module Tables.Player.State;

import Tables.Player.Type;
import std;

export namespace Tables::Player::State
{
	class PlayerTableStore
	{
	private:
		using AlivePlayer = Tables::Player::Type::Alive::Player;
		using PlayerTable = std::unordered_map<std::uint32_t, AlivePlayer>;

	public:
		PlayerTableStore() = default;
		~PlayerTableStore() = default;

		auto GetBase() const -> std::uintptr_t;
		auto SetBase(std::uintptr_t pointer) -> void;

		auto AddPlayer(std::uint32_t handle, const AlivePlayer& player) -> void;
		auto RemovePlayer(std::uint32_t handle) -> void;
		
		template<typename Predicate>
		auto RemovePlayerIf(Predicate&& predicate) -> void
		{
			std::lock_guard<std::mutex> lock(m_Mutex);

			for (auto it = m_PlayerTable.begin(); it != m_PlayerTable.end();)
			{
				if (predicate(it->first, it->second))
				{
					it = m_PlayerTable.erase(it);
				}
				else
				{
					++it;
				}
			}
		}

		template<typename Processor>
		auto UpdatePlayers(Processor&& processor) -> void
		{
			std::scoped_lock lock(m_Mutex);
			for (auto& [handle, player] : m_PlayerTable)
			{
				processor(handle, player);
			}
		}

		auto Publish() -> void;
		auto Acquire() const -> std::shared_ptr<const PlayerTable>;

		auto Cleanup() -> void;

	private:
		std::atomic<std::uintptr_t> m_PlayerTableBase{ 0 };
		std::atomic<std::shared_ptr<const PlayerTable>> m_pPlayerTable{};

		PlayerTable m_PlayerTable{};
		mutable std::mutex m_Mutex{};
	};
}