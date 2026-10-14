export module Environment.Entities.State;

export import :CarriedHandles;

import Common.State.System;
import Environment.Entities.Type;
import std;

export namespace Environment::Entities::State
{
    using EntitiesStore = Common::State::System::Snapshot<Environment::Entities::Type::Entities>;
}