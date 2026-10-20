export module Environment.Aim.State;

import Common.State.Type;
import Environment.Aim.Type;
import std;

export namespace Environment::Aim::State
{
    using Environment::Aim::Type::Aims;
    class AimStore : public Common::State::Type::Snapshot<Aims>
    {
    };
}