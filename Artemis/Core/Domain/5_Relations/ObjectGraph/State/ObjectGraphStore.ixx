export module Relations.ObjectGraph.State;

import Common.State.Type;
import Relations.ObjectGraph.Type;
import std;

namespace
{
    using Relations::ObjectGraph::Type::ObjectNode;
    using Relations::ObjectGraph::Type::ObjectNodes;
}

export namespace Relations::ObjectGraph::State
{
    class ObjectGraphStore : public Common::State::Type::Snapshot<ObjectNodes>
    {
    public:
        auto GetChildren(std::uint32_t handle) const -> std::vector<std::uint32_t>;
        auto GetParent(std::uint32_t handle) const -> std::uint32_t;
        auto GetSubtree(std::uint32_t rootHandle) const -> std::vector<std::uint32_t>;
    };
}