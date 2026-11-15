export module Perception.Aim.State;

export import :TagVitalities;

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