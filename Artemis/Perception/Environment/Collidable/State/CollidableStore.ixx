export module Environment.Collidable.State;

import Common.State.Type;
import Environment.Collidable.Type;
import std;

namespace
{
    using Environment::Collidable::Type::Collidables;
}

export namespace Environment::Collidable::State
{
    class CollidableStore : public Common::State::Type::Snapshot<Collidables>
    {
    };
}