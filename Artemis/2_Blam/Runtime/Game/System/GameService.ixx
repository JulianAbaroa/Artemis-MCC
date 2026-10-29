export module Blam.Runtime.Game.System;

import Common.Log.System;
import Blam.Layout.Snapshot.Type;
import Blam.Runtime.Game.Type;
import Blam.Runtime.Game.State;
import std;

export namespace Blam::Runtime::Game::System
{
    // Reads the game globals of the engine (clock, phase, scores and the event ring) into the store, once per tick (D103, D104).
    // note: Runs on the AI thread while the game waits for the read.
    class GameService
    {
    private:
        using Log = Common::Log::System::Log;

        using AliveGame = Blam::Runtime::Game::Type::Alive::AliveGame;
        using GameStore = Blam::Runtime::Game::State::GameStore;

        using Snapshot = Blam::Layout::Snapshot::Type::Snapshot;

    public:
        GameService(Log& log, GameStore& gameStore) : m_Log(log), m_GameStore(gameStore) {}
        ~GameService() = default;

        // Reads the globals of the tick from the snapshot and publishes them.
        // note: Does nothing before the thread context of the engine is known.
        auto Update(const Snapshot& snapshot) -> void;

        // Drops the published globals and resets the cursor of the event ring.
        auto Cleanup() -> void;

    private:
        Log& m_Log;
        GameStore& m_GameStore;

        // Reads the entries of the ring above the cursor into the globals and moves the cursor.
        // note: At most one ring of entries. The older ones were overwritten: they are counted and logged.
        auto ReadEvents(const Snapshot& snapshot, AliveGame& game) -> void;
    };
}