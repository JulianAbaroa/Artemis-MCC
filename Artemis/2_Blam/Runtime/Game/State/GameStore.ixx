export module Blam.Runtime.Game.State;

import Common.State.Type;
import Blam.Runtime.Game.Type;
import std;

namespace
{
    using Blam::Runtime::Game::Type::Alive::AliveGame;
}

export namespace Blam::Runtime::Game::State
{
    // Holds the game globals of the last tick and the cursor of the event ring.
    // note: The cursor and the dropped count are written by the AI thread only.
    class GameStore : public Common::State::Type::Snapshot<AliveGame>
    {
    public:
        // return: Last sequence of the event ring already read, 0 before the first one of the engine.
        auto GetEventCursor() const -> std::uint32_t;
        auto SetEventCursor(std::uint32_t sequence) -> void;

        // return: Entries of the event ring overwritten before they were read, since the engine started.
        auto GetDroppedEvents() const -> std::uint64_t;
        auto AddDroppedEvents(std::uint64_t count) -> void;

        // Drops the published globals and resets the cursor and the dropped count.
        auto Cleanup() -> void;

    private:
        std::atomic<std::uint32_t> m_EventCursor{ 0 };
        std::atomic<std::uint64_t> m_DroppedEvents{ 0 };
    };
}