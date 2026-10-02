export module Resolved.Definitions.Type:Eqip;

import :Object;
import std;

namespace
{
    using ResolvedObject = Resolved::Definitions::Type::Object::Object;
}

export namespace Resolved::Definitions::Type::Eqip
{
    struct Eqip
    {
        ResolvedObject Base{};
    };
}