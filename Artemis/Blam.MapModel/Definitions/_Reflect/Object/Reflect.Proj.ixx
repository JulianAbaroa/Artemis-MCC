export module Resolved.Definitions.Reflect:Proj;

import :Object;

import Common.Reflect.Type;
import Resolved.Definitions.Type;
import std;

namespace
{
    using Resolved::Definitions::Type::Proj::Proj;
}

export namespace Common::Reflect::Type
{
    template <>
    struct Fields<Proj>
    {
        static constexpr auto k_Value = std::tuple{
            MakeField("Base", &Proj::Base),
        };
    };
}