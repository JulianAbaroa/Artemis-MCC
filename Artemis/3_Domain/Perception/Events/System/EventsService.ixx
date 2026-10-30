export module Perception.Events.System;

import Blam.Runtime.Game.Type;
import Blam.Runtime.Game.State;
import Perception.Details.Type;
import Perception.Details.State;
import Perception.Events.Type;
import Perception.Events.State;
import std;

export namespace Perception::Events::System
{
    // Turns the entries of the event ring read this tick into events, adds the flag events Artemis generates from the transitions of the
    // flags, and keeps the last ones of the match (D104, D251).
    class EventsService
    {
    private:
        using AliveEvent = Blam::Runtime::Game::Type::Alive::AliveEvent;
        using AliveGame = Blam::Runtime::Game::Type::Alive::AliveGame;
        using GameEvent = Perception::Events::Type::GameEvent;
        using ObjectiveDetail = Perception::Details::Type::ObjectiveDetail;

        using GameStore = Blam::Runtime::Game::State::GameStore;
        using ObjectiveDetailsStore = Perception::Details::State::ObjectiveDetailsStore;
        using EventsStore = Perception::Events::State::EventsStore;
        using EventsMemoryStore = Perception::Events::State::EventsMemoryStore;

    public:
        EventsService(GameStore& gameStore, ObjectiveDetailsStore& objectiveDetailsStore, EventsMemoryStore& memoryStore, EventsStore& eventsStore) :
            m_GameStore(gameStore), m_ObjectiveDetailsStore(objectiveDetailsStore), m_MemoryStore(memoryStore), m_EventsStore(eventsStore) {}
        ~EventsService() = default;

        // Appends the events of the tick to the log and publishes it. Publishes nothing on a tick without events.
        // note: The engine events with the kind of a flag are left out: Artemis generates one flag event per real transition of the flag,
        // keyed by its base team. None is generated when the round changes or on the first tick after a load.
        // A carried flag that goes back home is a FlagCaptured only if the score of a team goes up in that tick or the next (the event
        // keeps the tick of the transition). Otherwise it is a FlagReset: the flag went home without scoring.
        // A first engine sequence at or below the last one read starts a new log (another match).
        auto Update() -> void;

        auto Cleanup() -> void;

    private:
        GameStore& m_GameStore;
        ObjectiveDetailsStore& m_ObjectiveDetailsStore;
        EventsMemoryStore& m_MemoryStore;
        EventsStore& m_EventsStore;

        static auto MakeEvent(const AliveEvent& entry) -> GameEvent;

        // Compares each flag with the one of the tick before and appends the flag events of the transitions.
        // note: Updates the memory, also when it generates nothing.
        auto AppendFlagEvents(const AliveGame& game, std::vector<GameEvent>& events) -> void;
    };
}