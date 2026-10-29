export module Views.Self.State;

import Common.State.Type;
import Views.Self.Type;

export namespace Views::Self::State
{
    class SelfStore : public Common::State::Type::Snapshot<Views::Self::Type::Self>
    {
    };
}