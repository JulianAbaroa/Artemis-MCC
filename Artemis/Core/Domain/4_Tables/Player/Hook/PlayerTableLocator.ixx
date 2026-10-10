export module Tables.Player.Hook;

import Service.Logs.System;
import Platform.Memory.System;
import Tables.Player.State;
import std;

export namespace Tables::Player::Hook
{
    // Finds the engine player table and stores its base address.
    class PlayerTableLocator
    {
    private:
        using LogsService = Service::Logs::System::LogsService;

        using AOBService = Platform::Memory::System::AOBService;

        using PlayerTableStore = Tables::Player::State::PlayerTableStore;

    public:
        PlayerTableLocator(LogsService& logsService, AOBService& aobService,
            PlayerTableStore& playerStore) : m_LogsService(logsService),
            m_AOBService(aobService), m_PlayerStore(playerStore) {}
        ~PlayerTableLocator() = default;

        // Stores the table base unless it is already stored.
        // note: Logs an error and stores nothing if the table is not found. Call it again later.
        auto FindAndStoreTableBase() -> void;

    private:
        LogsService& m_LogsService;
        AOBService& m_AOBService;
        PlayerTableStore& m_PlayerStore;

        // Finds the table that holds the session data of every player: names, state, camera and the handles of their biped, weapons and objective.
        // return: Address of the player table, or 0 if the thread context is not ready.
        // note: Reads the telemetry context through the thread local storage of the current thread.
        auto GetPlayerTable() -> std::uintptr_t;
    };
}