export module Perception.Events.State;

import Common.State.Type;
import Perception.Events.Type;
import std;

namespace
{
    using Perception::Events::Type::EventLog;
}

export namespace Perception::Events::State
{
    class EventsStore : public Common::State::Type::Snapshot<EventLog>
    {
    };
}