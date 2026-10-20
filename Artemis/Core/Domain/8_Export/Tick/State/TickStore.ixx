export module Export.Tick.State;

import Common.State.Type;
import Export.Tick.Type;

export namespace Export::Tick::State
{
    class TickStore : public Common::State::Type::Snapshot<Export::Tick::Type::Tick>
    {
    };
}