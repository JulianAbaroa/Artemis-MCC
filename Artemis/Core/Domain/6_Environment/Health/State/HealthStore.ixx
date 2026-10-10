export module Environment.Health.State;

import Common.State.System;
import Environment.Health.Type;
import std;

export namespace Environment::Health::State
{
    using Vitalities = std::unordered_map<std::uint32_t, Environment::Health::Type::Health>;
    using HealthStore = Common::State::System::Snapshot<Vitalities>;
}