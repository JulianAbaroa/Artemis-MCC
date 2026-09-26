export module Relations.ObjectGraph.State;

import Common.State.System;
import Relations.ObjectGraph.Type;
import std;

export namespace Relations::ObjectGraph::State
{
    using ObjectNode = Relations::ObjectGraph::Type::ObjectNode;
    using ObjectNodes = std::unordered_map<std::uint32_t, ObjectNode>;

    class ObjectGraphStore : public Common::State::System::Snapshot<ObjectNodes>
    {
    public:
        auto GetChildren(std::uint32_t handle) const -> std::vector<std::uint32_t>;
        auto GetParent(std::uint32_t handle) const -> std::uint32_t;
        auto GetSubtree(std::uint32_t rootHandle) const -> std::vector<std::uint32_t>;
    };
}