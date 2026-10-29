export module Perception.Players.State;

import Common.State.Type;
import Perception.Players.Type;
import std;

export namespace Perception::Players::State
{
    class PlayersStore : public Common::State::Type::Snapshot<Perception::Players::Type::Players>
    {
    };
}