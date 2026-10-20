export module Environment.Collidable.State;

import Common.State.Type;
import Environment.Collidable.Type;
import std;

export namespace Environment::Collidable::State
{
    using Environment::Collidable::Type::Collidables;
    class CollidableStore : public Common::State::Type::Snapshot<Collidables>
    {
    };
}