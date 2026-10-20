export module Environment.Aim.State;

import Common.State.Type;
import Environment.Aim.Type;
import std;

namespace
{
    using Environment::Aim::Type::Aims;
}

export namespace Environment::Aim::State
{
    class AimStore : public Common::State::Type::Snapshot<Aims>
    {
    };
}