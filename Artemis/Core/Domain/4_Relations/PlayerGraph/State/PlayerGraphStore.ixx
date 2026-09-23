export module Relations.PlayerGraph.State;

import Relations.PlayerGraph.Type;
import std;

export namespace Relations::PlayerGraph::State
{
	class PlayerGraphStore
	{
	private:
		using PlayerTree = Relations::PlayerGraph::Type::PlayerTree;
		using PlayerTrees = std::vector<PlayerTree>;

	public:
		PlayerGraphStore() = default;
		~PlayerGraphStore() = default;

		auto Publish(PlayerTrees trees) -> void;
		auto Acquire() const -> std::shared_ptr<const PlayerTrees>;

		auto Cleanup() -> void;

	private:
		std::atomic<std::shared_ptr<const PlayerTrees>> m_pPlayerTrees;
	};
}