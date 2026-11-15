module Perception.ObjectGraph.State;

import Common.Engine.Type;

namespace
{
    using Common::Engine::Type::k_NoHandle;
}

namespace Perception::ObjectGraph::State
{
    auto ObjectGraphStore::GetChildren(std::uint32_t handle) const -> std::vector<std::uint32_t>
    {
        auto snap = this->Acquire();
        if (!snap) return {};
        const auto* node = snap->Find(handle);
        if (node == nullptr) return {};
        const auto children = snap->ChildrenOf(*node);
        return std::vector<std::uint32_t>{ children.begin(), children.end() };
    }

    auto ObjectGraphStore::GetParent(std::uint32_t handle) const -> std::uint32_t
    {
        auto snap = this->Acquire();
        if (!snap) return k_NoHandle;
        const auto* node = snap->Find(handle);
        return node != nullptr ? node->ParentHandle : k_NoHandle;
    }

    auto ObjectGraphStore::GetSubtree(std::uint32_t rootHandle) const -> std::vector<std::uint32_t>
    {
        auto snap = this->Acquire();
        if (!snap) return {};

        std::vector<std::uint32_t> result;
        std::queue<std::uint32_t> pending;
        pending.push(rootHandle);

        while (!pending.empty())
        {
            std::uint32_t current = pending.front();
            pending.pop();
            result.push_back(current);

            const auto* node = snap->Find(current);
            if (node == nullptr) continue;

            for (std::uint32_t child : snap->ChildrenOf(*node))
            {
                pending.push(child);
            }
        }

        return result;
    }
}