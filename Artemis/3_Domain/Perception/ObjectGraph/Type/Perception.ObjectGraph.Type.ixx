export module Perception.ObjectGraph.Type;

import std;

export namespace Perception::ObjectGraph::Type
{
    // Parents walked up to the root of an object. Guards against a broken chain.
    inline constexpr int k_MaxParentDepth{ 16 };

    struct ObjectNode
    {
        std::uint32_t Handle{};
        std::uint32_t ParentHandle{};
        // The children are ObjectNodes::Children[FirstChild, FirstChild + ChildCount), in sibling order.
        std::uint32_t FirstChild{};
        std::uint32_t ChildCount{};
    };

    // Nodes of the object graph, sorted by handle, with the children of every node in one vector.
    struct ObjectNodes
    {
        std::vector<ObjectNode> Nodes{};
        std::vector<std::uint32_t> Children{};

        // return: The node of the handle, or null if the object is not live.
        auto Find(std::uint32_t handle) const -> const ObjectNode*
        {
            const auto it{ std::ranges::lower_bound(Nodes, handle, {}, &ObjectNode::Handle) };
            return (it == Nodes.end() || it->Handle != handle) ? nullptr : &*it;
        }

        // return: The handles of the children of the node, in sibling order. Valid as long as this value lives.
        auto ChildrenOf(const ObjectNode& node) const -> std::span<const std::uint32_t>
        {
            return std::span<const std::uint32_t>{ Children }.subspan(node.FirstChild, node.ChildCount);
        }
    };
}