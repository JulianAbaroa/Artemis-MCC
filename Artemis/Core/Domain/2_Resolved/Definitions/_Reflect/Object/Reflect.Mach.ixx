export module Resolved.Definitions.Reflect:Mach;

import :Object;

import Common.Reflect.Type;
import Resolved.Definitions.Type;
import std;

namespace
{
    using Mach = Resolved::Definitions::Type::Mach::Mach;
}

export namespace Common::Reflect::Type
{
    template <>
    struct Fields<Mach>
    {
        static constexpr bool HasFields = true;
        static constexpr auto Value = std::tuple{
            MakeField("Base", &Mach::Base),
        };
    };
}