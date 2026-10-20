export module Relations.ObjectGraph.Type;

import std;

export namespace Relations::ObjectGraph::Type
{
    // Parents walked up to the root of an object. Guards against a broken chain.
    inline constexpr int k_MaxParentDepth{ 16 };

    struct ObjectNode
    {
        std::uint32_t Handle{};
        std::uint32_t ParentHandle{};
        std::vector<std::uint32_t> ChildrenHandles{};
    };

    // Nodes of the object graph, by handle.
    struct ObjectNodes : std::unordered_map<std::uint32_t, ObjectNode>
    {
        using unordered_map::unordered_map;
    };
}