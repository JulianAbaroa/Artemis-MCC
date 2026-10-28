export module Environment.Entities.State;

export import :CarriedHandles;

import Common.State.Type;
import Environment.Entities.Type;
import std;

export namespace Environment::Entities::State
{
    class EntitiesStore : public Common::State::Type::Snapshot<Environment::Entities::Type::Entities>
    {
    };
}