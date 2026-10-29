export module Bridge.Codec.Tick.State;

import Common.State.Type;
import Bridge.Codec.Tick.Type;

export namespace Bridge::Codec::Tick::State
{
    class TickStore : public Common::State::Type::Snapshot<Bridge::Codec::Tick::Type::Tick>
    {
    };
}