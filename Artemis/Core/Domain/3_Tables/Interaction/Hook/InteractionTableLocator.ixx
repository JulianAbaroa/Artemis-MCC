export module Tables.Interaction.Hook;

import Service.Logs.System;
import Platform.Memory.System;
import Tables.Interaction.State;
import std;

export namespace Tables::Interaction::Hook
{
	class InteractionTableLocator
	{
	private:
		using LogsService = Service::Logs::System::LogsService;
		using AOBService = Platform::Memory::System::AOBService;
		using InteractionStore = Tables::Interaction::State::InteractionStore;

	public:
		InteractionTableLocator(LogsService& logsService, AOBService& aobService,
			InteractionStore& interactionStore) : m_LogsService(logsService),
			m_AOBService(aobService), m_InteractionStore(interactionStore) {}
		~InteractionTableLocator() = default;

		auto FindAndStoreTableBase() -> void;
		auto GetInteractionTable() -> std::uintptr_t;

	private:
		LogsService& m_LogsService;
		AOBService& m_AOBService;
		InteractionStore& m_InteractionStore;
	};
}