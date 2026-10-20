export module Egocentric.Affordance.State;

import Common.State.Type;
import Egocentric.Affordance.Type;
import std;

namespace
{
    using Egocentric::Affordance::Type::Affordances;
}

export namespace Egocentric::Affordance::State
{
    class AffordanceStore : public Common::State::Type::Snapshot<Affordances>
    {
    };
}