export module Tables.Player.Hook:CreatePlayer;

import Service.Logs.System;
import Platform.Memory.System;
import Tables.Player.System;
import std;

export namespace Tables::Player::Hook
{
	class CreatePlayerDetour
	{
	private:
		using LogsService = Service::Logs::System::LogsService;
		using AOBService = Platform::Memory::System::AOBService;
		using PlayerTableService = Tables::Player::System::PlayerTableService;

	public:
		CreatePlayerDetour(LogsService& logsService, AOBService& aobService,
			PlayerTableService& playerService) : m_LogsService(logsService),
			m_AOBService(aobService), m_PlayerService(playerService) {}
		~CreatePlayerDetour() = default;

		auto Install() -> void;
		auto Uninstall() -> void;

	private:
		LogsService& m_LogsService;
		AOBService& m_AOBService;
		PlayerTableService& m_PlayerService;

		static CreatePlayerDetour* s_Instance;

		static auto __fastcall HookedCreatePlayer(std::uint32_t playerIndex, 
			std::uint64_t pPlayerInfo, std::uint8_t playerFlags) -> std::uint32_t;

		typedef auto(__fastcall* Create_t)(std::uint32_t playerIndex, 
			std::uint64_t pPlayerInfo, std::uint8_t playerFlags) -> std::uint32_t;

		static inline Create_t m_OriginalFunction = nullptr;
		std::atomic<void*> m_FunctionAddress{ nullptr };
		std::atomic<bool> m_IsHookInstalled{ false };
	};
}