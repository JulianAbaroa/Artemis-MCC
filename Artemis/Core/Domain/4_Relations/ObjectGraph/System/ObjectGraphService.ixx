export module Relations.ObjectGraph.System;

import Service.Logs.System;
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
		using ObjectNodes = std::unordered_map<std::uint32_t, ObjectNode>;

		using LogsService = Service::Logs::System::LogsService;
		using ObjectTableStore = Tables::Object::State::ObjectTableStore;
		using ObjectGraphStore = Relations::ObjectGraph::State::ObjectGraphStore;

	public:
		ObjectGraphService(LogsService& logsService, ObjectTableStore& objectStore,
			ObjectGraphStore& objectGraphStore) : m_LogsService(logsService),
			m_ObjectStore(objectStore), m_ObjectGraphStore(objectGraphStore) {}
		~ObjectGraphService() = default;

		auto UpdateGraph() -> void;

		auto Cleanup() -> void;

	private:
		LogsService& m_LogsService;
		ObjectTableStore& m_ObjectStore;
		ObjectGraphStore& m_ObjectGraphStore;

		auto BuildNodes(ObjectNodes& nodes) -> void;
	};
}