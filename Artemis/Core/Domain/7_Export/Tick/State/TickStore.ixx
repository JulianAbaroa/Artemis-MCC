export module Export.Tick.State;

import Common.State.System;
import Export.Tick.Type;

export namespace Export::Tick::State
{
    using TickStore = Common::State::System::Snapshot<Export::Tick::Type::Tick>;
}