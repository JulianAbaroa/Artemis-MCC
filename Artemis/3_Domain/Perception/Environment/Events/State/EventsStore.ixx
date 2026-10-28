export module Environment.Events.State;

import Common.State.Type;
import Environment.Events.Type;
import std;

namespace
{
    using Environment::Events::Type::EventLog;
}

export namespace Environment::Events::State
{
    class EventsStore : public Common::State::Type::Snapshot<EventLog>
    {
    };
}