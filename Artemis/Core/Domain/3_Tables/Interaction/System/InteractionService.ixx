export module Tables.Interaction.System;

import Service.Logs.System;
import Platform.Memory.System;
import Tables.Interaction.State;

export namespace Tables::Interaction::System
{
	class InteractionService
	{
	private:
		using LogsService = Service::Logs::System::LogsService;
		using MemoryReaderService = Platform::Memory::System::MemoryReaderService;
		using InteractionStore = Tables::Interaction::State::InteractionStore;

	public:
		InteractionService(LogsService& logsService,
			MemoryReaderService& memoryReaderService,
			InteractionStore& interactionStore) :
			m_LogsService(logsService), m_MemoryReaderService(memoryReaderService),
			m_InteractionStore(interactionStore) {}
		~InteractionService() = default;

		auto UpdateInteractionTable() -> void;

		auto Cleanup() -> void;

	private:
		LogsService& m_LogsService;
		MemoryReaderService& m_MemoryReaderService;
		InteractionStore& m_InteractionStore;
	};
}