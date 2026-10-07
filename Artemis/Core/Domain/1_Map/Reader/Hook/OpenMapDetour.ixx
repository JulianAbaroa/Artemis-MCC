export module Map.Reader.Hook:OpenMap;

import Service.Logs.System;
import Platform.Memory.System;
import Map.Reader.Type;
import Map.Reader.State;
import Map.Reader.System;
import std;

export namespace Map::Reader::Hook
{
    // Hooks the engine map opening. Once the original returns, stores the path of the campaign and shared maps
    // and loads any other map.
    class OpenMapDetour
    {
    private:
        using OpenMapFunction = Map::Reader::Type::Hook::OpenMapFunction;

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

        // Finds the target by signature and installs the hook. Does nothing if already installed.
        auto Install() -> void;

        // Removes the hook once the calls in flight finish. Does nothing if not installed.
        auto Uninstall() -> void;

    private:
        LogsService& m_LogsService;
        AOBService& m_AOBService;
        FileStore& m_FileStore;
        MapLoaderService& m_MapLoaderService;

        static inline OpenMapDetour* s_Instance{ nullptr };
        static inline OpenMapFunction s_OriginalFunction{ nullptr };
        static inline std::atomic<int> s_InFlight{ 0 };

        std::atomic<void*> m_FunctionAddress{ nullptr };
        std::atomic<bool> m_IsHookInstalled{ false };

        static auto __fastcall HookedOpenMap(std::uint64_t param1,
            std::uint64_t param2, std::uint64_t mapRelativePath,
            std::uint32_t* param4) -> void;
    };
}