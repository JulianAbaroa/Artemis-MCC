export module Map.Reader.Hook:OpenMap;

import Service.Logs.System;
import Platform.Memory.System;
import Map.Reader.State;
import Map.Reader.System;
import std;

export namespace Map::Reader::Hook
{
	class OpenMapDetour
	{
	private:
		using LogsService = Service::Logs::System::LogsService;
		using AOBService = Platform::Memory::System::AOBService;
		using FileStore = Map::Reader::State::FileStore;
		using MapLoaderService = Map::Reader::System::MapLoaderService;

	public:
		OpenMapDetour(LogsService& logsService, AOBService& aobService,
			FileStore& fileStore, MapLoaderService& mapLoaderService) :
			m_LogsService(logsService), m_AOBService(aobService),
			m_FileStore(fileStore), m_MapLoaderService(mapLoaderService) {}
		~OpenMapDetour() = default;

		auto Install() -> void;
		auto Uninstall() -> void;

	private:
		LogsService& m_LogsService;
		AOBService& m_AOBService;
		FileStore& m_FileStore;
		MapLoaderService& m_MapLoaderService;

		static OpenMapDetour* s_Instance;

		static auto __fastcall HookedOpenMap(std::uint64_t param_1,
			std::uint64_t param_2, std::uint64_t mapRelativePath, 
			std::uint32_t* param_4) -> void;

		typedef auto(__fastcall* OpenMap_t)(
			std::uint64_t param_1, std::uint64_t param_2,
			std::uint64_t mapRelativePath, std::uint32_t* param_4) -> void;

		static inline OpenMap_t m_OriginalFunction = nullptr;
		std::atomic<void*> m_FunctionAddress{ nullptr };
		std::atomic<bool> m_IsHookInstalled{ false };
	};
}