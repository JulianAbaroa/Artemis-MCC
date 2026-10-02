export module Resolved.Definitions.Type:Scen;

import :Object;
import std;

namespace
{
    using ResolvedObject = Resolved::Definitions::Type::Object::Object;
}

export namespace Resolved::Definitions::Type::Scen
{
    struct Scen
    {
        ResolvedObject Base{};
    };
}