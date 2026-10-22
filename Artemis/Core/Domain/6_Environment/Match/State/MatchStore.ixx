export module Environment.Match.State;

import Common.State.Type;
import Environment.Match.Type;
import std;

namespace
{
    using Environment::Match::Type::MatchStatus;
}

export namespace Environment::Match::State
{
    class MatchStore : public Common::State::Type::Snapshot<MatchStatus>
    {
    };
}