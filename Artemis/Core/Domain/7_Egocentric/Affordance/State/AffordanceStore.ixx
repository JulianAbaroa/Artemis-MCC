export module Egocentric.Affordance.State;

import Common.State.System;
import Egocentric.Affordance.Type;
import std;

export namespace Egocentric::Affordance::State
{
    using Affordances = std::vector<Egocentric::Affordance::Type::Affordance>;
    using AffordanceStore = Common::State::System::Snapshot<Affordances>;
}