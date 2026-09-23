export module Runtime.Thread:AI;

import Service.Layer;
import Platform.Layer;
import Map.Layer;
import Resolved.Layer;
import Tables.Layer;
import Relations.Layer;
import Environment.Layer;
import Egocentric.Layer;
import Export.Layer;
import std;

export namespace Runtime::Thread
{
	class AIThread
	{
	public:
		AIThread(Service::Layer& service, Platform::Layer& platform, Map::Layer& map,
			Resolved::Layer& resolved, Tables::Layer& tables, Relations::Layer& relations,
			Environment::Layer& environment, Egocentric::Layer& egocentric,
			Export::Layer& exportLayer) :
			m_Service(service), m_Platform(platform), m_Map(map),
			m_Resolved(resolved), m_Tables(tables), m_Relations(relations),
			m_Environment(environment), m_Egocentric(egocentric),
			m_Export(exportLayer) {}
		~AIThread() = default;

		auto Run() -> void;

	private:
		Service::Layer& m_Service;
		Platform::Layer& m_Platform;
		Map::Layer& m_Map;
		Resolved::Layer& m_Resolved;
		Tables::Layer& m_Tables;
		Relations::Layer& m_Relations;
		Environment::Layer& m_Environment;
		Egocentric::Layer& m_Egocentric;
		Export::Layer& m_Export;

		bool m_WasLoaded{ false };

		std::uint64_t m_Last = 0;
		std::uint64_t m_Dropped = 0;

		auto LoadResources() -> void;

		auto ExecuteTick() -> void;

		auto IsStable() -> bool;

		auto Reset() -> void;
	};
}