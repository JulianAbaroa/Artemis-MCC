export module Resolved.Definitions.Reflect:Scen;

import :Object;

import Common.Reflect.Type;
import Resolved.Definitions.Type;
import std;

namespace
{
    using Scen = Resolved::Definitions::Type::Scen::Scen;
}

export namespace Common::Reflect::Type
{
    template <>
    struct Fields<Scen>
    {
        static constexpr bool HasFields = true;
        static constexpr auto Value = std::tuple{
            MakeField("Base", &Scen::Base),
        };
    };
}