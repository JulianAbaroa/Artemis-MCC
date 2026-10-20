export module Egocentric.Self.State;

import Common.State.Type;
import Egocentric.Self.Type;

export namespace Egocentric::Self::State
{
    class SelfStore : public Common::State::Type::Snapshot<Egocentric::Self::Type::Self>
    {
    };
}