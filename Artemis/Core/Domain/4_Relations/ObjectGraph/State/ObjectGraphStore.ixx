export module Relations.ObjectGraph.State;

import Relations.ObjectGraph.Type;
import std;

export namespace Relations::ObjectGraph::State
{
	class ObjectGraphStore
	{
	private:
		using ObjectNode = Relations::ObjectGraph::Type::ObjectNode;
		using ObjectNodes = std::unordered_map<std::uint32_t, ObjectNode>;

	public:
		ObjectGraphStore() = default;
		~ObjectGraphStore() = default;

		auto Publish(ObjectNodes nodes) -> void;
		auto Acquire() const -> std::shared_ptr<const ObjectNodes>;

		auto GetChildren(std::uint32_t handle) const -> std::vector<std::uint32_t>;
		auto GetParent(std::uint32_t handle) const -> std::uint32_t;
		auto GetSubtree(std::uint32_t rootHandle) const -> std::vector<std::uint32_t>;

		void Cleanup();

	private:
		std::atomic<std::shared_ptr<const ObjectNodes>> m_pObjectNodes;
	};
}