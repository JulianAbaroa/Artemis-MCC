export module Relations.ObjectGraph.System;

import Common.Log.System;
import Tables.Object.State;
import Relations.ObjectGraph.Type;
import Relations.ObjectGraph.State;
import std;

export namespace Relations::ObjectGraph::System
{
    class ObjectGraphService
    {
    private:
        using ObjectNode = Relations::ObjectGraph::Type::ObjectNode;
        using ObjectNodes = Relations::ObjectGraph::Type::ObjectNodes;

        using Log = Common::Log::System::Log;
        using ObjectTableStore = Tables::Object::State::ObjectTableStore;
        using ObjectGraphStore = Relations::ObjectGraph::State::ObjectGraphStore;

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