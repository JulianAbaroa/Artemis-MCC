export module Perception.Match.State;

import Common.State.Type;
import Perception.Match.Type;
import std;

namespace
{
    using Perception::Match::Type::MatchStatus;
}

export namespace Perception::Match::State
{
    class MatchStore : public Common::State::Type::Snapshot<MatchStatus>
    {
    };
}