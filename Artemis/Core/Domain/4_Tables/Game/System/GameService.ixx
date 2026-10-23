export module Tables.Game.System;

import Common.Log.System;
import Platform.Memory.System;
import Datum.Object.State;
import Tables.Game.Type;
import Tables.Game.State;
import std;

export namespace Tables::Game::System
{
    // Reads the game globals of the engine (clock, phase, scores and the event ring) into the store, once per tick (D103, D104).
    // note: Runs on the AI thread while the game waits for the read.
    class GameService
    {
    private:
        using Log = Common::Log::System::Log;

        using MemoryReaderService = Platform::Memory::System::MemoryReaderService;

        using DatumStore = Datum::Object::State::DatumStore;

        using AliveGame = Tables::Game::Type::Alive::AliveGame;
        using GameStore = Tables::Game::State::GameStore;

    public:
        GameService(Log& log, MemoryReaderService& memoryReaderService,
            DatumStore& datumStore, GameStore& gameStore) : m_Log(log),
            m_MemoryReaderService(memoryReaderService), m_DatumStore(datumStore), m_GameStore(gameStore) {}
        ~GameService() = default;

        // Reads the globals of the tick and publishes them.
        // note: Does nothing before the thread context of the engine is known.
        auto Update() -> void;

        // Drops the published globals and resets the cursor of the event ring.
        auto Cleanup() -> void;

    private:
        Log& m_Log;
        MemoryReaderService& m_MemoryReaderService;
        DatumStore& m_DatumStore;
        GameStore& m_GameStore;

        // Reads the entries of the ring above the cursor into the globals and moves the cursor.
        // note: At most one ring of entries. The older ones were overwritten: they are counted and logged.
        auto ReadEvents(std::uintptr_t eventGlobals, AliveGame& game) -> void;
    };
}