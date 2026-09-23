export module Tables.Object.Hook:ObjectTable;

import Service.Logs.System;
import Platform.Memory.System;
import Tables.Object.State;
import std;

export namespace Tables::Object::Hook
{
	class ObjectTableLocator
	{
	private:
		using LogsService = Service::Logs::System::LogsService;
		using AOBService = Platform::Memory::System::AOBService;
		using ObjectStore = Tables::Object::State::ObjectTableStore;

	public:
		ObjectTableLocator(LogsService& logsService, AOBService& aobService,
			ObjectStore& objectStore) : m_AOBService(aobService), 
			m_LogsService(logsService), m_ObjectStore(objectStore) {};
		~ObjectTableLocator() = default;

		auto FindAndStoreTableBase() -> void;
		auto GetObjectTable() -> std::uintptr_t;

	private:
		LogsService& m_LogsService;
		AOBService& m_AOBService;
		ObjectStore& m_ObjectStore;
	};
}