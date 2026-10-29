export module Perception.Events.System;

import Blam.Runtime.Game.Type;
import Blam.Runtime.Game.State;
import Perception.Events.Type;
import Perception.Events.State;
import std;

export namespace Perception::Events::System
{
    // Turns the entries of the event ring read this tick into events and keeps the last ones of the match (D104).
    class EventsService
    {
    private:
        using AliveEvent = Blam::Runtime::Game::Type::Alive::AliveEvent;
        using GameEvent = Perception::Events::Type::GameEvent;

        using GameStore = Blam::Runtime::Game::State::GameStore;
        using EventsStore = Perception::Events::State::EventsStore;

    public:
        EventsService(GameStore& gameStore, EventsStore& eventsStore) :
            m_GameStore(gameStore), m_EventsStore(eventsStore) {}
        ~EventsService() = default;

        // Appends the events of the tick to the log and publishes it. Publishes nothing on a tick without events.
        // note: A first sequence at or below the last one of the log starts a new log (another match).
        auto Update() -> void;

        auto Cleanup() -> void;

    private:
        GameStore& m_GameStore;
        EventsStore& m_EventsStore;

        static auto MakeEvent(const AliveEvent& entry) -> GameEvent;
    };
}