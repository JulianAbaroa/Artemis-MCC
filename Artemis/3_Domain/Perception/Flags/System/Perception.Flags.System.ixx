export module Perception.Flags.System;

import Blam.Runtime.Game.State;
import Blam.Runtime.Object.State;
import Perception.Classifier.State;
import Perception.Details.Type;
import Perception.Details.State;
import Perception.Events.System;
import Perception.Players.State;
import std;

export namespace Perception::Flags::System
{
    // Generates the flag events of the ticks that do not run the pipeline (sparse mode, Q101) from the light snapshot of the tick (D274, D278).
    class FlagPassService
    {
    private:
        using ObjectiveDetails = Perception::Details::Type::ObjectiveDetails;

        using ObjectTableStore = Blam::Runtime::Object::State::ObjectTableStore;
        using GameStore = Blam::Runtime::Game::State::GameStore;

        using TagClassificationsStore = Perception::Classifier::State::TagClassificationsStore;
        using ObjectiveDetailsStore = Perception::Details::State::ObjectiveDetailsStore;
        using PlayersStore = Perception::Players::State::PlayersStore;
        using EventsService = Perception::Events::System::EventsService;

    public:
        // param lightObjectStore, lightGameStore: What the light snapshot of the tick was decoded into (Blam.Runtime.Layer).
        // param objectiveDetailsStore, playersStore: What the last full tick published.
        FlagPassService(ObjectTableStore& lightObjectStore, GameStore& lightGameStore, TagClassificationsStore& tagClassificationsStore,
            ObjectiveDetailsStore& objectiveDetailsStore, PlayersStore& playersStore, EventsService& eventsService) :
            m_LightObjectStore(lightObjectStore), m_LightGameStore(lightGameStore), m_TagClassificationsStore(tagClassificationsStore),
            m_ObjectiveDetailsStore(objectiveDetailsStore), m_PlayersStore(playersStore), m_EventsService(eventsService) {}
        ~FlagPassService() = default;

        // return: The detail of each flag in the light tick, built with the same function as ObjectiveDetailsService from the weapons of the
        // light snapshot and what the last full tick published (stands, players and the tag classification). Without a full tick yet, none.
        // note: A weapon whose tag the full ticks never classified is not a flag. AlliesInZone is the one the last full tick gave the flag of
        // the same base team (D278). The carrier player is the owner of the carrier biped in the last full tick.
        auto BuildFlags() -> ObjectiveDetails;

        // Builds the flags of the light tick and hands them to the events service, which generates the transitions with its memory.
        // note: Runs on the AI thread after the light snapshot was decoded. Does nothing without a full tick before.
        auto Update() -> void;

    private:
        ObjectTableStore& m_LightObjectStore;
        GameStore& m_LightGameStore;
        TagClassificationsStore& m_TagClassificationsStore;
        ObjectiveDetailsStore& m_ObjectiveDetailsStore;
        PlayersStore& m_PlayersStore;
        EventsService& m_EventsService;
    };
}