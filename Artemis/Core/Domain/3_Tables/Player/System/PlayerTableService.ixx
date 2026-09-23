export module Tables.Player.System;

import Service.Logs.System;
import Platform.Memory.System;
import Tables.Player.Type;
import Tables.Player.State;
import std;

export namespace Tables::Player::System
{
	class PlayerTableService
	{
	private:
		using AlivePlayer = Tables::Player::Type::Alive::Player;

		using LogsService = Service::Logs::System::LogsService;
		using MemoryReaderService = Platform::Memory::System::MemoryReaderService;
		using PlayerTableStore = Tables::Player::State::PlayerTableStore;

	public:
		PlayerTableService(LogsService& logsService, 
			MemoryReaderService& memoryReaderService, PlayerTableStore& playerStore) : 
			m_LogsService(logsService), m_MemoryReaderService(memoryReaderService),
			m_PlayerStore(playerStore) {}
		~PlayerTableService() = default;

		auto OnPlayerCreated(std::uint32_t handle)-> void;

		auto UpdatePlayerTable() -> void;

		auto Cleanup() -> void;

	private:
		LogsService& m_LogsService;
		MemoryReaderService& m_MemoryReaderService;
		PlayerTableStore& m_PlayerStore;

		auto BuildLivePlayer(std::uint32_t handle, std::uintptr_t playerBase) -> AlivePlayer;
	
		auto UpdatePlayerData() -> void;

		// --- Helpers ---
		auto WideToUtf8(const wchar_t* source, std::size_t maxLength) -> std::string;
	};
}