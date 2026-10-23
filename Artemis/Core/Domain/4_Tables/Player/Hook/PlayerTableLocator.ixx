export module Tables.Player.Hook;

import Common.Log.System;
import Platform.Memory.System;
import Tables.Player.State;
import std;

export namespace Tables::Player::Hook
{
    // Finds the engine player table and the player mapping globals and stores their base addresses.
    class PlayerTableLocator
    {
    private:
        using Log = Common::Log::System::Log;

        using AOBService = Platform::Memory::System::AOBService;

        using PlayerTableStore = Tables::Player::State::PlayerTableStore;

    public:
        PlayerTableLocator(Log& log, AOBService& aobService,
            PlayerTableStore& playerStore) : m_Log(log),
            m_AOBService(aobService), m_PlayerStore(playerStore) {}
        ~PlayerTableLocator() = default;

        // Stores the table base and the mapping base unless they are already stored.
        // note: Logs an error and stores nothing for the one that is not found. Call it again later.
        auto FindAndStoreTableBase() -> void;

    private:
        Log& m_Log;
        AOBService& m_AOBService;
        PlayerTableStore& m_PlayerStore;

        // return: Thread context of the engine, or 0 if it is not ready.
        // note: Reads the thread local storage of the current thread. Call it from the game thread.
        auto GetThreadContext() -> std::uintptr_t;

        // Finds the table that holds the session data of every player: names, state, camera and the handles of their biped, weapons and objective.
        // return: Address of the player table, or 0 if the thread context is not ready.
        auto GetPlayerTable(std::uintptr_t threadContext) -> std::uintptr_t;

        // Finds the player mapping globals: which players are local (input users) and which player each view shows (output users).
        // return: Address of the mapping, or 0 if the thread context is not ready.
        auto GetMapping(std::uintptr_t threadContext) -> std::uintptr_t;
    };
}