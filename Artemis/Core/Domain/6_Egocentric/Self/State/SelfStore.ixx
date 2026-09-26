export module Egocentric.Self.State;

import Common.State.System;
import Egocentric.Self.Type;

export namespace Egocentric::Self::State
{
    using SelfStore = Common::State::System::Snapshot<Egocentric::Self::Type::Self>;
}