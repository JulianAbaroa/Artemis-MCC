export module Tables.Shadow.System:Game;

import Common.Log.System;
import Platform.Memory.System;
import Platform.Capture.State;
import Tables.Game.Type;
import Tables.Game.State;
import std;

export namespace Tables::Shadow::System
{
    // Old live read of the game globals, kept to compare it with the one that reads the snapshot (M4a, D188).
    // note: Temporary copy of the GameService of before the snapshot. M4b deletes it.
    class ShadowGameService
    {
    private:
        using Log = Common::Log::System::Log;

        using MemoryReaderService = Platform::Memory::System::MemoryReaderService;

        using CaptureStore = Platform::Capture::State::CaptureStore;

        using AliveGame = Tables::Game::Type::Alive::AliveGame;
        using GameStore = Tables::Game::State::GameStore;

    public:
        ShadowGameService(Log& log, MemoryReaderService& memoryReaderService,
            CaptureStore& captureStore, GameStore& gameStore) : m_Log(log),
            m_MemoryReaderService(memoryReaderService), m_CaptureStore(captureStore), m_GameStore(gameStore) {}
        ~ShadowGameService() = default;

        // Reads the globals of the tick and publishes them.
        // note: Does nothing before the thread context of the engine is known.
        auto Update() -> void;

        // Drops the published globals and resets the cursor of the event ring.
        auto Cleanup() -> void;

    private:
        Log& m_Log;
        MemoryReaderService& m_MemoryReaderService;
        CaptureStore& m_CaptureStore;
        GameStore& m_GameStore;

        // Reads the entries of the ring above the cursor into the globals and moves the cursor.
        // note: At most one ring of entries. The older ones were overwritten: they are counted and logged.
        auto ReadEvents(std::uintptr_t eventGlobals, AliveGame& game) -> void;
    };
}