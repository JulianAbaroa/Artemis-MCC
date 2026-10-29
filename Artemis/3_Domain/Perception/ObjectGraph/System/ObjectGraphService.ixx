export module Perception.ObjectGraph.System;

import Common.Log.System;
import Blam.Runtime.Object.State;
import Perception.ObjectGraph.Type;
import Perception.ObjectGraph.State;
import std;

export namespace Perception::ObjectGraph::System
{
    class ObjectGraphService
    {
    private:
        using ObjectNode = Perception::ObjectGraph::Type::ObjectNode;
        using ObjectNodes = Perception::ObjectGraph::Type::ObjectNodes;

        using Log = Common::Log::System::Log;
        using ObjectTableStore = Blam::Runtime::Object::State::ObjectTableStore;
        using ObjectGraphStore = Perception::ObjectGraph::State::ObjectGraphStore;

    public:
        ObjectGraphService(Log& log, ObjectTableStore& objectStore,
            ObjectGraphStore& objectGraphStore) : m_Log(log),
            m_ObjectStore(objectStore), m_ObjectGraphStore(objectGraphStore) {}
        ~ObjectGraphService() = default;

        auto UpdateGraph() -> void;

        auto Cleanup() -> void;

    private:
        Log& m_Log;
        ObjectTableStore& m_ObjectStore;
        ObjectGraphStore& m_ObjectGraphStore;

        auto BuildNodes(ObjectNodes& nodes) -> void;
    };
}