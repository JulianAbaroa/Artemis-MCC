module Perception.ObjectGraph.System;

import Common.Engine.Type;
import Blam.Runtime.Object.Type;

namespace
{
    using Blam::Runtime::Object::Type::Alive::AliveObject;
    using Blam::Runtime::Object::Type::Alive::ObjectTable;

    using Common::Engine::Type::k_NoHandle;
}

namespace Perception::ObjectGraph::System
{
    auto ObjectGraphService::UpdateGraph() -> void
    {
        ObjectNodes nodes;
        this->BuildNodes(nodes);
        m_ObjectGraphStore.Publish(std::move(nodes));
    }

    auto ObjectGraphService::BuildNodes(ObjectNodes& nodes) -> void
    {
        auto objectTablePtr = m_ObjectStore.Acquire();
        if (!objectTablePtr) return;
        const ObjectTable& objectTable = *objectTablePtr;

        for (const auto& [handle, object] : objectTable)
        {
            ObjectNode node{};
            node.Handle = handle;
            node.ParentHandle = object.ParentHandle;

            std::uint32_t childHandle = object.ChildHandle;
            while (childHandle != k_NoHandle)
            {
                auto it = objectTable.find(childHandle);
                if (it == objectTable.end()) break;

                node.ChildrenHandles.push_back(childHandle);
                childHandle = it->second.NextSiblingHandle;
            }

            nodes.emplace(handle, std::move(node));
        }
    }

    auto ObjectGraphService::Cleanup() -> void
    {
        m_ObjectGraphStore.Cleanup();

        m_Log.Message("[ObjectGraphService] INFO: Cleanup completed.");
    }
}