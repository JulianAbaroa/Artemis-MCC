export module Tables.Player.Hook:PlayerTable;

import Service.Logs.System;
import Platform.Memory.System;
import Tables.Player.State;
import std;

export namespace Tables::Player::Hook
{
	class PlayerTableLocator
	{
	private:
		using LogsService = Service::Logs::System::LogsService;
		using AOBService = Platform::Memory::System::AOBService;
		using PlayerStore = Tables::Player::State::PlayerTableStore;

	public:
		PlayerTableLocator(LogsService& logsService, AOBService& aobService,
			PlayerStore& playerStore) : m_LogsService(logsService),
			m_AOBService(aobService), m_PlayerStore(playerStore) {}
		~PlayerTableLocator() = default;

		auto FindAndStoreTableBase() -> void;
		auto GetPlayerTable() -> std::uintptr_t;

	private:
		LogsService& m_LogsService;
		AOBService& m_AOBService;
		PlayerStore& m_PlayerStore;
	};
}