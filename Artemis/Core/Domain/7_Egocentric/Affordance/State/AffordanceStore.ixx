export module Egocentric.Affordance.State;

import Common.State.Type;
import Egocentric.Affordance.Type;
import std;

export namespace Egocentric::Affordance::State
{
    using Egocentric::Affordance::Type::Affordances;
    class AffordanceStore : public Common::State::Type::Snapshot<Affordances>
    {
    };
}