module Relations.ObjectGraph.State;

import Common.Engine.Type;

namespace
{
    using Common::Engine::Type::k_NoHandle;
}

namespace Relations::ObjectGraph::State
{
    auto ObjectGraphStore::GetChildren(std::uint32_t handle) const -> std::vector<std::uint32_t>
    {
        auto snap = this->Acquire();
        if (!snap) return {};
        auto it = snap->find(handle);
        if (it == snap->end()) return {};
        return it->second.ChildrenHandles;
    }

    auto ObjectGraphStore::GetParent(std::uint32_t handle) const -> std::uint32_t
    {
        auto snap = this->Acquire();
        if (!snap) return k_NoHandle;
        auto it = snap->find(handle);
        return it != snap->end() ? it->second.ParentHandle : k_NoHandle;
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

            auto it = snap->find(current);
            if (it == snap->end()) continue;

            for (std::uint32_t child : it->second.ChildrenHandles)
            {
                pending.push(child);
            }
        }

        return result;
    }
}