export module Perception.Entities.State;

export import :CarriedHandles;

import Common.State.Type;
import Perception.Entities.Type;
import std;

export namespace Perception::Entities::State
{
    class EntitiesStore : public Common::State::Type::Snapshot<Perception::Entities::Type::Entities>
    {
    };
}