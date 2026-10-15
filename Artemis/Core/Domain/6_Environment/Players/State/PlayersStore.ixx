export module Environment.Players.State;

import Common.State.System;
import Environment.Players.Type;
import std;

export namespace Environment::Players::State
{
    using PlayersStore = Common::State::System::Snapshot<Environment::Players::Type::Players>;
}