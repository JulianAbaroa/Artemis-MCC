export module Environment.Fixtures.State;

import Common.State.System;
import Environment.Fixtures.Type;

export namespace Environment::Fixtures::State
{
    using FixturesStore = Common::State::System::Snapshot<Environment::Fixtures::Type::Fixtures>;
}