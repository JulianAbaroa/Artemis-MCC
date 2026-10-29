export module Views.Affordance.State;

import Common.State.Type;
import Views.Affordance.Type;
import std;

namespace
{
    using Views::Affordance::Type::Affordances;
}

export namespace Views::Affordance::State
{
    class AffordanceStore : public Common::State::Type::Snapshot<Affordances>
    {
    };
}