export module Resolved.Definitions.Reflect:Bipd;

import :Object;

import Common.Reflect.Type;
import Resolved.Definitions.Type;
import std;

namespace
{
    using Bipd = Resolved::Definitions::Type::Bipd::Bipd;
}

export namespace Common::Reflect::Type
{
    template <>
    struct Fields<Bipd>
    {
        static constexpr bool HasFields = true;
        static constexpr auto Value = std::tuple{
            MakeField("Base", &Bipd::Base),
        };
    };
}