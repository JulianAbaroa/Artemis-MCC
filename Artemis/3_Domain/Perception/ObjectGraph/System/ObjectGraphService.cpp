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

        nodes.Nodes.reserve(objectTable.size());

        for (const auto& [handle, object] : objectTable)
        {
            ObjectNode node{};
            node.Handle = handle;
            node.ParentHandle = object.ParentHandle;
            node.FirstChild = static_cast<std::uint32_t>(nodes.Children.size());

            std::uint32_t childHandle = object.ChildHandle;
            while (childHandle != k_NoHandle)
            {
                auto it = objectTable.find(childHandle);
                if (it == objectTable.end()) break;

                nodes.Children.push_back(childHandle);
                childHandle = it->second.NextSiblingHandle;
            }

            node.ChildCount = static_cast<std::uint32_t>(nodes.Children.size()) - node.FirstChild;
            nodes.Nodes.push_back(node);
        }

        std::ranges::sort(nodes.Nodes, {}, &ObjectNode::Handle);
    }

    auto ObjectGraphService::Cleanup() -> void
    {
        m_ObjectGraphStore.Cleanup();

        m_Log.Message("[ObjectGraphService] INFO: Cleanup completed.");
    }
}