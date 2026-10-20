export module Environment.Players.State;

import Common.State.Type;
import Environment.Players.Type;
import std;

export namespace Environment::Players::State
{
    class PlayersStore : public Common::State::Type::Snapshot<Environment::Players::Type::Players>
    {
    };
}