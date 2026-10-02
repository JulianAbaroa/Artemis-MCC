export module Resolved.Definitions.Type:Bloc;

import :Object;
import std;

namespace
{
    using ResolvedObject = Resolved::Definitions::Type::Object::Object;
}

export namespace Resolved::Definitions::Type::Bloc
{
    struct Bloc
    {
        ResolvedObject Base{};
    };
}