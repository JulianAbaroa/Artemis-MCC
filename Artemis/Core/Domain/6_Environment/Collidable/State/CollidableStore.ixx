export module Environment.Collidable.State;

import Common.State.System;
import Environment.Collidable.Type;
import std;

export namespace Environment::Collidable::State
{
    using Collidables = std::vector<Environment::Collidable::Type::Collidable>;
    using CollidableStore = Common::State::System::Snapshot<Collidables>;
}