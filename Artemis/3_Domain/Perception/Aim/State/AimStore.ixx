export module Perception.Aim.State;

import Common.State.Type;
import Perception.Aim.Type;
import std;

namespace
{
    using Perception::Aim::Type::Aims;
}

export namespace Perception::Aim::State
{
    class AimStore : public Common::State::Type::Snapshot<Aims>
    {
    };
}